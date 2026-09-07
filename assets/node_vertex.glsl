#version 460 core

struct Node {
    vec4 position;
    vec4 velocity;
    vec4 color;
};

layout(std430, binding = 0) buffer b_nodes_block {
    Node nodes[];
};

uniform vec2 u_resolution;
uniform float u_zoom;
uniform vec2 u_screen_pos;

out vec4 nodeColor;
out vec2 localPos;

void main()
{
    Node node = nodes[gl_InstanceID];

    float baseRadius = 0.2;

    float minRadiusPixels = 2.0;

    float pixelWorld = 2.0 / (u_resolution.y * u_zoom);

    float radius = max(
        baseRadius,
        minRadiusPixels * pixelWorld
    );

    vec2 offsets[6] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 1.0, -1.0),
        vec2( 1.0,  1.0),

        vec2(-1.0, -1.0),
        vec2( 1.0,  1.0),
        vec2(-1.0,  1.0)
    );

    vec2 position =
    node.position.xy +
    offsets[gl_VertexID] * radius;

    vec2 p = position - u_screen_pos;

    p *= u_zoom;

    float aspect = u_resolution.x / u_resolution.y;
    p.x /= aspect;

    gl_Position = vec4(p, 0.0, 1.0);

    localPos = offsets[gl_VertexID];

    nodeColor = node.color;
}