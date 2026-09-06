#version 460 core

struct Node {
    vec2 pos;
    vec3 col;
};

layout(location = 0) in vec2 aPosition;

uniform vec2 u_resolution;
uniform float u_zoom;
uniform vec2 u_screen_pos;

out vec2 uv;
out vec2 resolution;

out float zoom;
out vec2 screen_pos;

void main()
{
    uv = aPosition * 0.5 + 0.5;
    resolution = u_resolution;
    zoom = u_zoom;
    screen_pos = u_screen_pos;

    gl_Position = vec4(aPosition, 0.0, 1.0);
}