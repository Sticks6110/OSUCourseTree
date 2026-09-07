#include <iostream>
#include <random>
#include <SDL3/SDL.h>
#include <glad/glad.h>

#include "ComputeShader.h"
#include "courses.h"
#include "node.h"
#include "Shader.h"

//Some Code Is Borrowed From My Other Project: https://github.com/Sticks6110/SticksEngine/tree/main

int main() {
    std::cout << "Hello, World!" << std::endl;

    //Initialize SDL with Video flag
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "SDL_Init failed!" << std::endl;
        return -1;
    }

    //Set GL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24); //RGB8

    //Create a resizable Window
    SDL_Window *window = SDL_CreateWindow("OSU Course Tree", 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cout << "SDL_CreateWindow failed!" << std::endl;
        return -1;
    }

    //Create the GL Context and get them ready for rendering
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress);

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    //Create the quad to cover the screen
    float vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,

        -1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };

    GLuint VAO;
    GLuint VBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices),vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * sizeof(float),
        nullptr
    );

    glBindVertexArray(0);

    //Load Catalog and Generate Graph
    courses* catalog = new courses("assets/osu_courses_2026_2027_processed.json");
    Graph graph = catalog->generate_graph();

    //Create the buffer for the nodes
    unsigned int ssboNodes;
    glGenBuffers(1, &ssboNodes);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboNodes);

    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 graph.nodes.size() * sizeof(Node),
                 graph.nodes.data(),
                 GL_DYNAMIC_DRAW);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboNodes);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    //Create the buffer for the edges
    unsigned int ssboEdges;
    glGenBuffers(1, &ssboEdges);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboEdges);

    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 graph.edges.size() * sizeof(Edge),
                 graph.edges.data(),
                 GL_DYNAMIC_DRAW);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboEdges);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    //Selected Node Buffer
    uint32_t no_selection = UINT32_MAX;
    GLuint selectedNodeBuffer;

    glGenBuffers(1, &selectedNodeBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, selectedNodeBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t), &no_selection, GL_DYNAMIC_READ);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, selectedNodeBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    //Create the shaders
    Shader node_shader("assets/node_vertex.glsl", "assets/node_fragment.glsl");
    node_shader.use();
    GLint node_resolution_uniform = glGetUniformLocation(node_shader.program_id, "u_resolution");
    GLint node_zoom_uniform = glGetUniformLocation(node_shader.program_id, "u_zoom");
    GLint node_screen_pos_uniform = glGetUniformLocation(node_shader.program_id, "u_screen_pos");

    Shader edge_shader("assets/edge_vertex.glsl", "assets/edge_fragment.glsl");
    edge_shader.use();
    GLint edge_resolution_uniform = glGetUniformLocation(edge_shader.program_id, "u_resolution");
    GLint edge_zoom_uniform = glGetUniformLocation(edge_shader.program_id, "u_zoom");
    GLint edge_screen_pos_uniform = glGetUniformLocation(edge_shader.program_id, "u_screen_pos");

    ComputeShader force_shader("assets/force.glsl");
    force_shader.use();
    glUniform1ui(glGetUniformLocation(force_shader.program_id, "nodeCount"), graph.nodes.size());
    glUniform1ui(glGetUniformLocation(force_shader.program_id, "edgeCount"), graph.edges.size());
    glUniform1f(glGetUniformLocation(force_shader.program_id, "dt"), 0.056f);
    glUniform1f(glGetUniformLocation(force_shader.program_id, "repulsion"), 5.0f);
    glUniform1f(glGetUniformLocation(force_shader.program_id, "springStrength"), 0.5f);
    glUniform1f(glGetUniformLocation(force_shader.program_id, "springLength"), 5.0f);
    glUniform1f(glGetUniformLocation(force_shader.program_id, "damping"), 0.9f);
    glUniform1f(glGetUniformLocation(force_shader.program_id, "centeringStrength"), 0.01f);

    GLuint groups = (graph.nodes.size() + 255) / 256;

    ComputeShader select_shader("assets/select_node.glsl");

    ///
    /// MAIN LOOP
    ///
    ///

    //Control Tracking
    bool is_panning = false;
    glm::vec2 camera(0.0);
    float zoom = 12;
    float zoom_processed = 0.02;

    //FPS Tracking
    Uint64 freq = SDL_GetPerformanceFrequency();
    Uint64 last = SDL_GetPerformanceCounter();
    float time = 0.0;
    bool running = true;
    while (running) {
        ///
        /// FPS LIMITING
        ///
        Uint64 now = SDL_GetPerformanceCounter();
        double delta = (double) (now - last) * 1000.0 / freq;
        float deltaTime = delta / 1000.0;
        time += deltaTime;

        if (delta < 1000.0 / 60.0) {
            SDL_Delay(0);
            continue;
        }

        last = now;

        ///
        /// BASIC DATA
        ///

        int w;
        int h;
        SDL_GetWindowSize(window, &w, &h);

        ///
        /// Event Handling
        ///
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if (e.button.button == SDL_BUTTON_MIDDLE || e.button.button == SDL_BUTTON_RIGHT) {
                        is_panning = true;
                    }

                    if (e.button.button == SDL_BUTTON_LEFT) {
                        float mouse_x = e.button.x;
                        float mouse_y = h - e.button.y;

                        float aspect = (float)w / h;

                        float mouse_ndc_x = mouse_x / w * 2.0f - 1.0f;
                        float mouse_ndc_y = mouse_y / h * 2.0f - 1.0f;

                        mouse_ndc_x *= aspect;

                        glm::vec2 mouse_world(
                            mouse_ndc_x / zoom_processed + camera.x,
                            mouse_ndc_y / zoom_processed + camera.y
                        );

                        float baseRadius = 0.2;

                        float minRadiusPixels = 2.0;

                        float pixelWorld = 2.0 / (h * zoom_processed);

                        float radius = std::max(
                            baseRadius,
                            minRadiusPixels * pixelWorld
                        );

                        std::cout << "CLICKED " << mouse_world.x << " " << mouse_world.y << std::endl;

                        select_shader.use();

                        glBindBuffer(GL_SHADER_STORAGE_BUFFER, selectedNodeBuffer);
                        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &no_selection);

                        glUniform1ui(glGetUniformLocation(select_shader.program_id, "nodeCount"), graph.nodes.size());
                        glUniform2f(glGetUniformLocation(select_shader.program_id, "mouseWorld"), mouse_world.x, mouse_world.y);
                        glUniform1f(glGetUniformLocation(select_shader.program_id, "selectionRadius"), radius);

                        glDispatchCompute((graph.nodes.size() + 255) / 256, 1, 1);
                        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

                        uint32_t selectedNode;

                        glBindBuffer(GL_SHADER_STORAGE_BUFFER, selectedNodeBuffer);
                        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &selectedNode);

                        if (selectedNode != UINT32_MAX) {
                            std::cout << "Selected node: " << graph.courses[selectedNode] << " " << selectedNode << std::endl;
                        }
                    }
                    break;

                case SDL_EVENT_MOUSE_BUTTON_UP:
                    if (e.button.button == SDL_BUTTON_MIDDLE || e.button.button == SDL_BUTTON_RIGHT) {
                        is_panning = false;
                    }
                    break;

                case SDL_EVENT_MOUSE_MOTION:
                    if (is_panning) {
                        camera.x -= e.motion.xrel / w / zoom_processed;
                        camera.y += e.motion.yrel / h / zoom_processed;
                    }
                    break;
                case SDL_EVENT_MOUSE_WHEEL:
                    zoom -= e.wheel.y;
                    zoom_processed = 1.0 / (zoom * zoom);
                    std::cout << zoom_processed << " " << zoom << std::endl;
                    break;
            }
        }

        ///
        /// Rendering
        ///

        glViewport(0, 0, w, h);
        //glClearColor(144.0f / 255.0f, 213.0f / 255.0f, 1.0f, 1.0f);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Compute the force shader
        force_shader.use();
        for (int i = 0; i < 10; ++i) {
            glDispatchCompute(groups, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        // Draw Edges
        edge_shader.use();
        glUniform2f(edge_resolution_uniform, (float)w, (float)h);
        glUniform1f(edge_zoom_uniform, zoom_processed);
        glUniform2f(edge_screen_pos_uniform, camera.x, camera.y);

        glBindVertexArray(VAO);

        glDrawArraysInstanced(
            GL_TRIANGLE_STRIP,
            0,
            6,
            graph.edges.size()
        );

        // Draw Nodes
        node_shader.use();
        glUniform2f(node_resolution_uniform, (float)w, (float)h);
        glUniform1f(node_zoom_uniform, zoom_processed);
        glUniform2f(node_screen_pos_uniform, camera.x, camera.y);

        glBindVertexArray(VAO);

        glDrawArraysInstanced(
            GL_TRIANGLES,
            0,
            6,
            graph.nodes.size()
        );

        SDL_GL_SwapWindow(window);
    }

    // cleanup
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}