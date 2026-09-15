#ifndef OSUCOURSETREE_NODE_H
#define OSUCOURSETREE_NODE_H
#include <glm/glm.hpp>

struct Node {
    Node(glm::vec2 pos, glm::vec2 vel, uint32_t level_value, uint32_t connection_count, glm::vec3 node_color)
        : position(pos, 0.0f, 0.0f), velocity(vel, 0.0f, 0.0f), color(node_color, 1.0f),
          level(level_value), connections(connection_count), padding{} {}

    glm::vec4 position;
    glm::vec4 velocity;
    glm::vec4 color;
    uint32_t level;
    uint32_t connections;
    uint32_t padding[2];
};

struct Edge {
    Edge(uint32_t source, uint32_t destination) : a(source), b(destination) {}

    uint32_t a;
    uint32_t b;
};

#endif
