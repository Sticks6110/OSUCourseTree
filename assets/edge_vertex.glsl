#version 460 core

struct Node {
    vec4 position;
    vec4 velocity;
    vec4 color;
    uint level;
    uint connections;
};

layout(std430, binding = 0) buffer b_nodes_block {
    Node nodes[];
};

struct Edge {
    uint a;
    uint b;
};

layout(std430, binding = 1) buffer b_edges_block {
    Edge edges[];
};

uniform vec2 u_resolution;
uniform float u_zoom;
uniform vec2 u_screen_pos;

void main()
{
    Edge edge = edges[gl_InstanceID];

    vec2 a = nodes[edge.a].position.xy;
    vec2 b = nodes[edge.b].position.xy;

    vec2 delta = b - a;
    float len = length(delta);

    if (len < 0.00001) {
        gl_Position = vec4(2.0, 2.0, 0.0, 1.0);
        return;
    }

    vec2 dir = delta / len;

    vec2 normal = vec2(-dir.y, dir.x);

    float minWidthPixels = 0.5;

    float pixelWorld = 2.0 / (u_resolution.y * u_zoom);

    float halfWidth = max(0.03, minWidthPixels * pixelWorld);

    vec2 position;

    if (gl_VertexID == 0) position = a - normal * halfWidth;
    else if (gl_VertexID == 1) position = b - normal * halfWidth;
    else if (gl_VertexID == 2) position = a + normal * halfWidth;
    else position = b + normal * halfWidth;

    vec2 p = position - u_screen_pos;

    p *= u_zoom;

    float aspect = u_resolution.x / u_resolution.y;
    p.x /= aspect;

    gl_Position = vec4(p, 0.0, 1.0);
}