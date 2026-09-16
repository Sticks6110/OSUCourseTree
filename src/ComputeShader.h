#ifndef OSUCOURSETREE_COMPUTESHADER_H
#define OSUCOURSETREE_COMPUTESHADER_H

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif


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
