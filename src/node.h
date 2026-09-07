#ifndef OSUCOURSETREE_NODE_H
#define OSUCOURSETREE_NODE_H
#include <glm/glm.hpp>

struct Node {
public:
    Node(glm::vec2 pos, glm::vec2 vel, glm::vec3 color) : position(pos, 0.0, 0.0), velocity(vel, 0.0, 0.0), color(color, 0.0) {

    }

    glm::vec4 position;
    glm::vec4 velocity;
    glm::vec4 color;
};

struct Edge {
public:
    Edge(uint32_t a, uint32_t b) : a(a), b(b) {

    }

    uint32_t a;
    uint32_t b;
};

#endif