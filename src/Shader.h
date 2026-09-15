#ifndef OSUCOURSETREE_SHADER_H
#define OSUCOURSETREE_SHADER_H


#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <glad/glad.h>
#include <stdexcept>

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
