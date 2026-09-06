#include <iostream>
#include <random>
#include <SDL3/SDL.h>
#include <glad/glad.h>

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

    //Create / Get the nodes for the graph
    // std::vector<Node> nodes;
    // std::vector<Edge> edges;
    //
    // std::random_device rd;
    // std::mt19937 gen(rd());
    // std::uniform_int_distribution<int> pos_distrib(-50, 50);
    // std::uniform_real_distribution<double> col_distrib(0.0, 1.0);
    //
    // for (int i = 0; i < 1000; i++) {
    //     nodes.push_back(Node(
    //         glm::vec2(i, pos_distrib(gen)),
    //         glm::vec3(col_distrib(gen), col_distrib(gen), col_distrib(gen))));
    //
    //     if (i != 0) {
    //         edges.push_back(Edge(i - 1, i));
    //     }
    // }

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

    //Create the shader
    Shader shader("assets/vertex.glsl", "assets/fragment.glsl");
    GLint resolution_uniform = glGetUniformLocation(shader.program_id, "u_resolution");
    GLint zoom_uniform = glGetUniformLocation(shader.program_id, "u_zoom");
    GLint screen_pos_uniform = glGetUniformLocation(shader.program_id, "u_screen_pos");

    shader.use(); //The only shader in the project for now.

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
        glClearColor(144.0f / 255.0f, 213.0f / 255.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUniform2f(resolution_uniform, (float)w, (float)h);
        glUniform1f(zoom_uniform, zoom_processed);
        glUniform2f(screen_pos_uniform, camera.x, camera.y);

        glBindVertexArray(VAO);

        glDrawArrays(
            GL_TRIANGLES,
            0,
            6
        );

        SDL_GL_SwapWindow(window);
    }

    // cleanup
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}