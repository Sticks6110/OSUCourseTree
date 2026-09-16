#ifndef OSUCOURSETREE_SHADER_H
#define OSUCOURSETREE_SHADER_H


#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

class Shader {
public:
    GLuint program_id = 0;

    Shader(const GLchar *vertex_path, const GLchar *frag_path);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&&) = delete;
    Shader& operator=(Shader&&) = delete;

    void use() const;
};


#endif
