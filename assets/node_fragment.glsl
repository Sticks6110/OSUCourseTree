#version 460 core

struct Node {
    vec2 pos;
    vec3 col;
};

layout(std140, binding = 0) buffer b_nodes_block {
    Node nodes[];
};

struct Edge {
    uint a;
    uint b;
};

layout(std430, binding = 1) buffer b_edges_block {
    Edge edges[];
};

in vec2 uv;
in vec2 resolution;

in float zoom;
in vec2 screen_pos;

out vec4 FragColor;

float lineDistance(vec2 p, vec2 a, vec2 b)
{
    vec2 ab = b - a;
    vec2 ap = p - a;

    float t = dot(ap, ab) / dot(ab, ab);
    t = clamp(t, 0.0, 1.0);

    vec2 closest = a + t * ab;

    return distance(p, closest);
}

void main()
{
    vec2 st = gl_FragCoord.xy / resolution.xy;
    st -= vec2(0.5);
    st.x *= resolution.x / resolution.y;
    st /= zoom;
    st += screen_pos;

    vec3 color = vec3(0.0);

    float radius = 0.2;

    for (int i = 0; i < nodes.length(); i++) {
        vec2 nodePos = nodes[i].pos;

        float dist = distance(st, nodePos);

        float circle = smoothstep(
            radius,
            radius - 0.002,
            dist
        );

        color = mix(color, nodes[i].col, circle);
    }

    for (int i = 0; i < edges.length(); i++) {
        vec2 a = nodes[edges[i].a].pos;
        vec2 b = nodes[edges[i].b].pos;

        float dist = lineDistance(st, a, b);

        float lineWidth = 0.001 / zoom;

        float line = smoothstep(
            lineWidth * 2.0,
            lineWidth,
            dist
        );

        color = mix(color, vec3(1.0), line);
    }

    FragColor = vec4(color, 1.0);
}