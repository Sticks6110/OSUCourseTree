#version 460 core

struct Node {
    vec4 position;
    vec4 velocity;
    vec4 color;
};

struct Edge {
    uint a;
    uint b;
};

layout(std430, binding = 0) buffer NodeBuffer {
    Node nodes[];
};

layout(std430, binding = 1) readonly buffer EdgeBuffer {
    Edge edges[];
};

uniform uint nodeCount;
uniform uint edgeCount;

uniform float dt;

uniform float repulsion;
uniform float springStrength;
uniform float springLength;
uniform float damping;
uniform float centeringStrength;

layout(local_size_x = 256) in;

void main()
{
    uint i = gl_GlobalInvocationID.x;

    if (i >= nodeCount)
    return;

    vec2 position = nodes[i].position.xy;
    vec2 force = vec2(0.0);

    for (uint j = 0; j < nodeCount; ++j) {
        if (i == j)
        continue;

        vec2 delta = position - nodes[j].position.xy;

        float dist2 = dot(delta, delta);

        // Prevent division by zero and huge forces
        dist2 = max(dist2, 0.01);

        float invDist = inversesqrt(dist2);

        // Unit vector pointing away from j
        vec2 direction = delta * invDist;

        // F = k / r^2
        float strength = repulsion / dist2;

        force += direction * strength;
    }

    for (uint e = 0; e < edgeCount; ++e) {
        Edge edge = edges[e];

        uint other;

        if (edge.a == i)
        other = edge.b;
        else if (edge.b == i)
        other = edge.a;
        else
        continue;

        vec2 delta = nodes[other].position.xy - position;

        float dist = length(delta);

        if (dist < 0.0001)
        continue;

        vec2 direction = delta / dist;

        // force = k * (distance - desiredLength)
        float displacement = dist - springLength;

        force += direction * displacement * springStrength;
    }

    force -= position * centeringStrength; // Center

    vec2 velocity = nodes[i].velocity.xy;

    velocity += force * dt;

    // Damping
    velocity *= damping;

    position += velocity * dt;

    nodes[i].position.xy = position;
    nodes[i].velocity.xy = velocity;
}