#include "graph_window.h"

#include "ComputeShader.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "Shader.h"
#include "glad/glad.h"

graph_window::graph_window(std::string title, Graph graph, courses* catalog) : title(title), graph(graph), catalog(catalog), node_shader(Shader("assets/node_vertex.glsl", "assets/node_fragment.glsl")), edge_shader(Shader("assets/edge_vertex.glsl", "assets/edge_fragment.glsl")), force_shader(ComputeShader("assets/force.glsl")), select_shader(ComputeShader("assets/select_node.glsl"))  {
    camera = glm::vec2(0, 0);

    float vertices[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f, 1.0f,

        -1.0f, -1.0f,
        1.0f, 1.0f,
        -1.0f, 1.0f
    };

    //Create the FBO
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Create texture to render into
    glGenTextures(1, &textureColorBuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBuffer, 0);

    // Check if complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) std::cout << "Framebuffer is not complete!" << std::endl;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

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

    //Create the buffer for the nodes
    glGenBuffers(1, &ssboNodes);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboNodes);

    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 graph.nodes.size() * sizeof(Node),
                 graph.nodes.data(),
                 GL_DYNAMIC_DRAW);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboNodes);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    //Create the buffer for the edges
    glGenBuffers(1, &ssboEdges);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboEdges);

    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 graph.edges.size() * sizeof(Edge),
                 graph.edges.data(),
                 GL_DYNAMIC_DRAW);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboEdges);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    //Selected Node Buffer
    glGenBuffers(1, &selectedNodeBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, selectedNodeBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t), &no_selection, GL_DYNAMIC_READ);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, selectedNodeBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    //Create the shaders
    node_shader.use();
    node_resolution_uniform = glGetUniformLocation(node_shader.program_id, "u_resolution");
    node_zoom_uniform = glGetUniformLocation(node_shader.program_id, "u_zoom");
    node_screen_pos_uniform = glGetUniformLocation(node_shader.program_id, "u_screen_pos");

    edge_shader.use();
    edge_resolution_uniform = glGetUniformLocation(edge_shader.program_id, "u_resolution");
    edge_zoom_uniform = glGetUniformLocation(edge_shader.program_id, "u_zoom");
    edge_screen_pos_uniform = glGetUniformLocation(edge_shader.program_id, "u_screen_pos");

    force_shader.use();
    force_node_count_uniform = glGetUniformLocation(force_shader.program_id, "nodeCount");
    force_edge_count_uniform = glGetUniformLocation(force_shader.program_id, "edgeCount");
    force_dt_uniform = glGetUniformLocation(force_shader.program_id, "dt");
    force_repulsion_uniform = glGetUniformLocation(force_shader.program_id, "repulsion");
    force_spring_strength_uniform = glGetUniformLocation(force_shader.program_id, "springStrength");
    force_spring_length_uniform = glGetUniformLocation(force_shader.program_id, "springLength");
    force_damping_uniform = glGetUniformLocation(force_shader.program_id, "damping");
    force_centering_strength_uniform = glGetUniformLocation(force_shader.program_id, "centeringStrength");

    glUniform1ui(force_node_count_uniform, graph.nodes.size());
    glUniform1ui(force_edge_count_uniform, graph.edges.size());
    glUniform1f(force_dt_uniform, 0.1f);

    glUniform1f(force_repulsion_uniform, repulsion);
    glUniform1f(force_spring_strength_uniform, spring_strength);
    glUniform1f(force_spring_length_uniform, spring_length);
    glUniform1f(force_damping_uniform, spring_damping);
    glUniform1f(force_centering_strength_uniform, centering_strength);

    groups = (graph.nodes.size() + 255) / 256;
}

graph_window::~graph_window()
{
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &textureColorBuffer);

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glDeleteBuffers(1, &ssboNodes);
    glDeleteBuffers(1, &ssboEdges);
    glDeleteBuffers(1, &selectedNodeBuffer);
}

void graph_window::update(float deltaTime) {
    if (!open) return;

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Compute the force shader
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboNodes);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboEdges);

    force_shader.use();
    glUniform1f(force_dt_uniform, std::min(0.1f, deltaTime));

    if (force_data_dirty) {
        glUniform1f(force_repulsion_uniform, repulsion);
        glUniform1f(force_spring_strength_uniform, spring_strength);
        glUniform1f(force_spring_length_uniform, spring_length);
        glUniform1f(force_damping_uniform, spring_damping);
        glUniform1f(force_centering_strength_uniform, centering_strength);
        force_data_dirty = false;
    }

    for (int i = 0; i < 10; ++i) {
        glDispatchCompute(groups, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    // Draw Edges
    edge_shader.use();
    glUniform2f(edge_resolution_uniform, (float)width, (float)height);
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
    glUniform2f(node_resolution_uniform, (float)width, (float)height);
    glUniform1f(node_zoom_uniform, zoom_processed);
    glUniform2f(node_screen_pos_uniform, camera.x, camera.y);

    glBindVertexArray(VAO);

    glDrawArraysInstanced(
        GL_TRIANGLES,
        0,
        6,
        graph.nodes.size()
    );

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ImGui::Begin(title.c_str(), &open);

    ImVec2 windowSize = ImGui::GetContentRegionAvail();

    ImGui::Image((ImTextureID)(intptr_t)textureColorBuffer, windowSize, ImVec2(0, 1), ImVec2(1, 0));

    graph_screen_pos = ImGui::GetItemRectMin();

    if (width != windowSize.x || height != windowSize.y) {
        resize_fbo(windowSize.x, windowSize.y);
    }

    width = windowSize.x;
    height = windowSize.y;

    ImGui::End();

}

void graph_window::process_event(SDL_Event &e) {
    if (!open) return;

    switch (e.type) {
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
                camera.x -= e.motion.xrel / width / zoom_processed;
                camera.y += e.motion.yrel / height / zoom_processed;
            }
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            zoom -= e.wheel.y;
            zoom_processed = 1.0 / (zoom * zoom);
            break;
    }
}

Course graph_window::select_node_at_location(float mouse_x, float mouse_y) {
    if (!open) return Course{};

    bool inside_graph =
        mouse_x >= graph_screen_pos.x &&
        mouse_x <= graph_screen_pos.x + width &&
        mouse_y >= graph_screen_pos.y &&
        mouse_y <= graph_screen_pos.y + height;

    if (!inside_graph) return Course{};

    ImVec2 mouse_screen_pos = ImGui::GetMousePos();

    ImVec2 mouse_local_pos = ImVec2(mouse_screen_pos.x - graph_screen_pos.x, mouse_screen_pos.y - graph_screen_pos.y);

    std::cout << mouse_local_pos.x << ", " << mouse_local_pos.y << std::endl;

    float aspect = (float)width / height;

    float mouse_ndc_x = mouse_local_pos.x / width * 2.0f - 1.0f;
    float mouse_ndc_y = 1.0f - mouse_local_pos.y / height * 2.0f;

    mouse_ndc_x *= aspect;

    glm::vec2 mouse_world(
        mouse_ndc_x / zoom_processed + camera.x,
        mouse_ndc_y / zoom_processed + camera.y
    );

    float baseRadius = 0.2;

    float minRadiusPixels = 2.0;

    float pixelWorld = 2.0 / (height * zoom_processed);

    float radius = std::max(
        baseRadius,
        minRadiusPixels * pixelWorld
    );

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboNodes);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, selectedNodeBuffer);

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
        return catalog->catalog[graph.courses[selectedNode]];
    }

    return Course{};
}

void graph_window::set_physics_settings(float repulsion, float spring_strength, float spring_length, float spring_damping, float centering_strength) {
    if (!open) return;

    glUniform1f(force_repulsion_uniform, repulsion);
    glUniform1f(force_spring_strength_uniform, spring_strength);
    glUniform1f(force_spring_length_uniform, spring_length);
    glUniform1f(force_damping_uniform, spring_damping);
    glUniform1f(force_centering_strength_uniform, centering_strength);
    force_data_dirty = true;
}

void graph_window::resize_fbo(int width, int height) {
    if (!open) return;

    glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer failed to resize!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
