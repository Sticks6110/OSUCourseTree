#ifndef OSUCOURSETREE_NODE_H
#define OSUCOURSETREE_NODE_H
#include <glm/glm.hpp>

struct Node {
public:
    Node(glm::vec2 pos, glm::vec2 vel, uint32_t level, uint32_t connections, glm::vec3 color) : position(pos, 0.0, 0.0), velocity(vel, 0.0, 0.0), level(level), connections(connections), color(color, 0.0) {

    }

    glm::vec4 position;
    glm::vec4 velocity;
    glm::vec4 color;
    uint32_t level;
    uint32_t connections;
    uint32_t padding[2];
};

struct Edge {
public:
    Edge(uint32_t a, uint32_t b) : a(a), b(b) {

    }

    uint32_t a;
    uint32_t b;
};

#endif