#include "Shader.h"

#include <array>

namespace {

std::string read_text_file(const GLchar* path) {
    std::ifstream file(path, std::ios::in);
    if (!file) {
        throw std::runtime_error(std::string("Unable to open shader file: ") + path);
    }

    std::stringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

GLuint compile_shader(GLenum type, const std::string& source, const char* label) {
    const GLchar* shader_source = source.c_str();
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &shader_source, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) {
        return shader;
    }

    std::array<GLchar, 2048> log{};
    glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
    glDeleteShader(shader);
    throw std::runtime_error(std::string("Unable to compile ") + label + " shader: " + log.data());
}

} // namespace

Shader::Shader(const GLchar *vertex_path, const GLchar *frag_path) {
    const GLuint vertex = compile_shader(GL_VERTEX_SHADER, read_text_file(vertex_path), "vertex");
    GLuint fragment = 0;
    try {
        fragment = compile_shader(GL_FRAGMENT_SHADER, read_text_file(frag_path), "fragment");
    } catch (...) {
        glDeleteShader(vertex);
        throw;
    }
    program_id = glCreateProgram();
    glAttachShader(program_id, vertex);
    glAttachShader(program_id, fragment);
    glLinkProgram(program_id);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint linked = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
        std::array<GLchar, 2048> log{};
        glGetProgramInfoLog(program_id, static_cast<GLsizei>(log.size()), nullptr, log.data());
        glDeleteProgram(program_id);
        program_id = 0;
        throw std::runtime_error(std::string("Unable to link shader program: ") + log.data());
    }
}

Shader::~Shader() {
    if (program_id != 0) glDeleteProgram(program_id);
}

void Shader::use() const {
    glUseProgram(program_id);
}
