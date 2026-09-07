#version 460 core

in vec4 nodeColor;
in vec2 localPos;

out vec4 FragColor;

void main()
{
    float dist = length(localPos);

    if (dist > 1.0) discard;

    FragColor = nodeColor;
}