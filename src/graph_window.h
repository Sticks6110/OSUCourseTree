#ifndef OSUCOURSETREE_GRAPH_WINDOW_H
#define OSUCOURSETREE_GRAPH_WINDOW_H
#include "ComputeShader.h"
#include "courses.h"
#include "Shader.h"
#include "glad/glad.h"
#include <SDL3/SDL.h>

#include "imgui.h"

class graph_window {
public:
    graph_window(std::string title, Graph graph, const courses* catalog);
    ~graph_window();

    void update(float deltaTime);
    void process_event(const SDL_Event& event);
    [[nodiscard]] Course select_node_at_location(float mouse_x, float mouse_y);
    void set_physics_settings(float repulsion, float spring_strength, float spring_length, float spring_damping, float centering_strength);
    [[nodiscard]] bool is_open() const noexcept;

private:
    void resize_fbo(int width, int height);
    bool is_mouse_over();

    std::string title;
    bool open = true;

    const courses* catalog;
    Graph graph;

    GLuint fbo = 0;
    GLuint textureColorBuffer = 0;
    int width = 800;
    int height = 600;

    GLuint VAO = 0;
    GLuint VBO = 0;

    bool force_data_dirty = false;
    float repulsion = 30.0f;
    float spring_strength = 0.05f;
    float spring_length = 10.0f;
    float spring_damping = 0.9f;
    float centering_strength = 0.01f;

    GLuint ssboNodes = 0;
    GLuint ssboEdges = 0;

    uint32_t no_selection = UINT32_MAX;
    GLuint selectedNodeBuffer = 0;

    bool is_panning = false;
    glm::vec2 camera{0.0f};
    float zoom = 12.0f;
    float zoom_processed = 1.0f / (12.0f * 12.0f);

    ImVec2 graph_screen_pos{0.0f, 0.0f};

    Shader node_shader;
    Shader edge_shader;
    ComputeShader force_shader;
    ComputeShader select_shader;

    GLuint groups = 0;

    GLint node_resolution_uniform;
    GLint node_zoom_uniform;
    GLint node_screen_pos_uniform;

    GLint edge_resolution_uniform;
    GLint edge_zoom_uniform;
    GLint edge_screen_pos_uniform;

    GLint force_node_count_uniform;
    GLint force_edge_count_uniform;
    GLint force_dt_uniform;
    GLint force_repulsion_uniform;
    GLint force_spring_strength_uniform;
    GLint force_spring_length_uniform;
    GLint force_damping_uniform;
    GLint force_centering_strength_uniform;
};


#endif
