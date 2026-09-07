#ifndef OSUCOURSETREE_COMPUTESHADER_H
#define OSUCOURSETREE_COMPUTESHADER_H
#include <glad/glad.h>


class ComputeShader {
public:
    GLuint program_id;

    ComputeShader(const GLchar *shader_path);

    void use();
};


#endif