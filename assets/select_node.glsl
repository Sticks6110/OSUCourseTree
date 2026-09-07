#version 460 core

struct Node {
    vec4 position;
    vec4 velocity;
    vec4 color;
};

layout(std430, binding = 0) readonly buffer NodeBuffer {
    Node nodes[];
};

layout(std430, binding = 3) buffer SelectionBuffer {
    uint selectedNode;
};

uniform uint nodeCount;
uniform vec2 mouseWorld;
uniform float selectionRadius;

layout(local_size_x = 256) in;

void main()
{
    uint i = gl_GlobalInvocationID.x;

    if (i >= nodeCount)
    return;

    vec2 delta = mouseWorld - nodes[i].position.xy;

    if (dot(delta, delta) <= selectionRadius * selectionRadius) {
        atomicMin(selectedNode, i);
    }
}