#include "graph_window.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace {
constexpr float kMinimumZoom = 1.5f;
constexpr float kMaximumZoom = 60.0f;

bool contains(const ImVec2 origin, int width, int height, float x, float y) {
    return x >= origin.x && x <= origin.x + width && y >= origin.y && y <= origin.y + height;
}
} // namespace

graph_window::graph_window(std::string title, Graph graph, const courses* catalog)
    : title(std::move(title)), catalog(catalog), graph(std::move(graph)),
      node_shader("assets/node_vertex.glsl", "assets/node_fragment.glsl"),
      edge_shader("assets/edge_vertex.glsl", "assets/edge_fragment.glsl"),
      force_shader("assets/force.glsl"), select_shader("assets/select_node.glsl") {
    constexpr float vertices[] = {
        -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f,
    };

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGenTextures(1, &textureColorBuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) throw std::runtime_error("Unable to create graph framebuffer.");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glBindVertexArray(0);

    glGenBuffers(1, &ssboNodes);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboNodes);
    glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(this->graph.nodes.size() * sizeof(Node)), this->graph.nodes.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboNodes);
    glGenBuffers(1, &ssboEdges);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboEdges);
    glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(this->graph.edges.size() * sizeof(Edge)), this->graph.edges.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboEdges);
    glGenBuffers(1, &selectedNodeBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, selectedNodeBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(no_selection), &no_selection, GL_DYNAMIC_READ);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, selectedNodeBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

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
    glUniform1ui(force_node_count_uniform, static_cast<GLuint>(this->graph.nodes.size()));
    glUniform1ui(force_edge_count_uniform, static_cast<GLuint>(this->graph.edges.size()));
    glUniform1f(force_dt_uniform, 0.1f);
    glUniform1f(force_repulsion_uniform, repulsion);
    glUniform1f(force_spring_strength_uniform, spring_strength);
    glUniform1f(force_spring_length_uniform, spring_length);
    glUniform1f(force_damping_uniform, spring_damping);
    glUniform1f(force_centering_strength_uniform, centering_strength);
    groups = static_cast<GLuint>((this->graph.nodes.size() + 255U) / 256U);
}

graph_window::~graph_window() {
    glDeleteBuffers(1, &selectedNodeBuffer);
    glDeleteBuffers(1, &ssboEdges);
    glDeleteBuffers(1, &ssboNodes);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteTextures(1, &textureColorBuffer);
    glDeleteFramebuffers(1, &fbo);
}

bool graph_window::is_open() const noexcept { return open; }

void graph_window::update(float deltaTime) {
    if (!open) return;
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, width, height);
    glClearColor(0.025f, 0.035f, 0.055f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (groups > 0) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboNodes);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboEdges);
        force_shader.use();
        glUniform1f(force_dt_uniform, std::clamp(deltaTime, 0.0f, 0.1f));
        if (force_data_dirty) {
            glUniform1f(force_repulsion_uniform, repulsion);
            glUniform1f(force_spring_strength_uniform, spring_strength);
            glUniform1f(force_spring_length_uniform, spring_length);
            glUniform1f(force_damping_uniform, spring_damping);
            glUniform1f(force_centering_strength_uniform, centering_strength);
            force_data_dirty = false;
        }
        for (int iteration = 0; iteration < 10; ++iteration) {
            glDispatchCompute(groups, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }
    }

    edge_shader.use();
    glUniform2f(edge_resolution_uniform, static_cast<float>(width), static_cast<float>(height));
    glUniform1f(edge_zoom_uniform, zoom_processed);
    glUniform2f(edge_screen_pos_uniform, camera.x, camera.y);
    glBindVertexArray(VAO);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(graph.edges.size()));
    node_shader.use();
    glUniform2f(node_resolution_uniform, static_cast<float>(width), static_cast<float>(height));
    glUniform1f(node_zoom_uniform, zoom_processed);
    glUniform2f(node_screen_pos_uniform, camera.x, camera.y);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<GLsizei>(graph.nodes.size()));
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ImGui::Begin(title.c_str(), &open);
    const ImVec2 window_size = ImGui::GetContentRegionAvail();
    const int next_width = std::max(1, static_cast<int>(window_size.x));
    const int next_height = std::max(1, static_cast<int>(window_size.y));
    ImGui::Image(static_cast<ImTextureID>(textureColorBuffer), ImVec2(std::max(1.0f, window_size.x), std::max(1.0f, window_size.y)), ImVec2(0, 1), ImVec2(1, 0));
    graph_screen_pos = ImGui::GetItemRectMin();
    if (width != next_width || height != next_height) resize_fbo(next_width, next_height);
    width = next_width;
    height = next_height;
    ImGui::End();
}

void graph_window::process_event(const SDL_Event& event) {
    if (!open) return;
    const bool hover_graph = [&] {
        switch (event.type) {
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP: return contains(graph_screen_pos, width, height, event.button.x, event.button.y);
            case SDL_EVENT_MOUSE_MOTION: return contains(graph_screen_pos, width, height, event.motion.x, event.motion.y);
            case SDL_EVENT_MOUSE_WHEEL: {
                const ImVec2 mouse = ImGui::GetMousePos();
                return contains(graph_screen_pos, width, height, mouse.x, mouse.y);
            }
            default: return false;
        }
    }();
    switch (event.type) {
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (hover_graph && (event.button.button == SDL_BUTTON_MIDDLE || event.button.button == SDL_BUTTON_RIGHT)) is_panning = true;
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_MIDDLE || event.button.button == SDL_BUTTON_RIGHT) is_panning = false;
            break;
        case SDL_EVENT_MOUSE_MOTION:
            if (is_panning) {
                camera.x -= event.motion.xrel / static_cast<float>(width) / zoom_processed;
                camera.y += event.motion.yrel / static_cast<float>(height) / zoom_processed;
            }
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            if (hover_graph) {
                zoom = std::clamp(zoom - event.wheel.y, kMinimumZoom, kMaximumZoom);
                zoom_processed = 1.0f / (zoom * zoom);
            }
            break;
    }
}

Course graph_window::select_node_at_location(float mouse_x, float mouse_y) const {
    if (!open || graph.nodes.empty() || !contains(graph_screen_pos, width, height, mouse_x, mouse_y)) return {};
    const float local_x = mouse_x - graph_screen_pos.x;
    const float local_y = mouse_y - graph_screen_pos.y;
    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    const float mouse_ndc_x = (local_x / width * 2.0f - 1.0f) * aspect;
    const float mouse_ndc_y = 1.0f - local_y / height * 2.0f;
    const glm::vec2 mouse_world(mouse_ndc_x / zoom_processed + camera.x, mouse_ndc_y / zoom_processed + camera.y);
    const float radius = std::max(0.2f, 4.0f / (height * zoom_processed));
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboNodes);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, selectedNodeBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, selectedNodeBuffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(no_selection), &no_selection);
    select_shader.use();
    glUniform1ui(glGetUniformLocation(select_shader.program_id, "nodeCount"), static_cast<GLuint>(graph.nodes.size()));
    glUniform2f(glGetUniformLocation(select_shader.program_id, "mouseWorld"), mouse_world.x, mouse_world.y);
    glUniform1f(glGetUniformLocation(select_shader.program_id, "selectionRadius"), radius);
    glDispatchCompute(static_cast<GLuint>((graph.nodes.size() + 255U) / 256U), 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    uint32_t selected_node = no_selection;
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(selected_node), &selected_node);
    const auto course_code = graph.courses.find(selected_node);
    if (course_code == graph.courses.end()) return {};
    const Course* selected_course = catalog->find_course(course_code->second);
    return selected_course != nullptr ? *selected_course : Course{};
}

void graph_window::set_physics_settings(float new_repulsion, float new_spring_strength, float new_spring_length, float new_spring_damping, float new_centering_strength) {
    repulsion = new_repulsion;
    spring_strength = new_spring_strength;
    spring_length = new_spring_length;
    spring_damping = new_spring_damping;
    centering_strength = new_centering_strength;
    force_data_dirty = true;
}

void graph_window::resize_fbo(int new_width, int new_height) {
    glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, new_width, new_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}
