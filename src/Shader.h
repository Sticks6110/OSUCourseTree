#ifndef OSUCOURSETREE_SHADER_H
#define OSUCOURSETREE_SHADER_H


#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <glad/glad.h>
#include <glm/mat4x4.hpp>

//#include "imgui_impl_opengl3_loader.h"
//#include "imgui_impl_sdl3.h"

class Shader {
public:
    GLuint program_id;

    Shader(const GLchar *vertex_path, const GLchar *frag_path);

    void use();
};


#endif