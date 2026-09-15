#ifndef OSUCOURSETREE_GRAPH_WINDOW_H
#define OSUCOURSETREE_GRAPH_WINDOW_H
#include "ComputeShader.h"
#include "courses.h"
#include "shader.h"
#include "glad/glad.h"
#include <SDL3/SDL.h>

#include "imgui.h"

class graph_window {
public:
    graph_window(std::string title, Graph graph, courses* catalog);
    ~graph_window();

    void change_graph(Graph& graph);
    void update(float deltaTime);
    void process_event(SDL_Event& e);
    Course select_node_at_location(float mouse_x, float mouse_y);
    void set_physics_settings(float repulsion, float spring_strength, float spring_length, float spring_damping, float centering_strength);

private:
    void resize_fbo(int width, int height);

    std::string title;
    bool open = true;

    courses* catalog;
    Graph graph;

    GLuint fbo;
    GLuint textureColorBuffer;
    int width = 800;
    int height = 600;

    GLuint VAO;
    GLuint VBO;

    bool force_data_dirty = false;
    float repulsion = 30.0f;
    float spring_strength = 0.05f;
    float spring_length = 10.0f;
    float spring_damping = 0.9f;
    float centering_strength = 0.01f;

    unsigned int ssboNodes;
    unsigned int ssboEdges;

    uint32_t no_selection = UINT32_MAX;
    GLuint selectedNodeBuffer;

    bool is_panning = false;
    glm::vec2 camera;
    float zoom = 12;
    float zoom_processed = 0.02;

    ImVec2 graph_screen_pos;

    Shader node_shader;
    Shader edge_shader;
    ComputeShader force_shader;
    ComputeShader select_shader;

    GLuint groups;

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