#include "ComputeShader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <array>
#include <stdexcept>

namespace {

std::string read_text_file(const GLchar* path) {
    std::ifstream file(path, std::ios::in);
    if (!file) throw std::runtime_error(std::string("Unable to open compute shader file: ") + path);

    std::stringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

} // namespace

ComputeShader::ComputeShader(const GLchar *shader_path) {
    const std::string code = read_text_file(shader_path);
    const GLchar* shader_code = code.c_str();
    const GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &shader_code, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
        std::array<GLchar, 2048> log{};
        glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error(std::string("Unable to compile compute shader: ") + log.data());
    }

    program_id = glCreateProgram();
    glAttachShader(program_id, shader);
    glLinkProgram(program_id);
    glDeleteShader(shader);

    GLint linked = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
        std::array<GLchar, 2048> log{};
        glGetProgramInfoLog(program_id, static_cast<GLsizei>(log.size()), nullptr, log.data());
        glDeleteProgram(program_id);
        program_id = 0;
        throw std::runtime_error(std::string("Unable to link compute shader program: ") + log.data());
    }
}

ComputeShader::~ComputeShader() {
    if (program_id != 0) glDeleteProgram(program_id);
}

void ComputeShader::use() const {
    glUseProgram(program_id);
}
