#ifndef OSUCOURSETREE_COMPUTESHADER_H
#define OSUCOURSETREE_COMPUTESHADER_H
#include <glad/glad.h>


class ComputeShader {
public:
    GLuint program_id = 0;

    ComputeShader(const GLchar *shader_path);
    ~ComputeShader();

    ComputeShader(const ComputeShader&) = delete;
    ComputeShader& operator=(const ComputeShader&) = delete;
    ComputeShader(ComputeShader&&) = delete;
    ComputeShader& operator=(ComputeShader&&) = delete;

    void use() const;
};


#endif
