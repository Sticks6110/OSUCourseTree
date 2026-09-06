//
// Created by beast on 8/30/2026.
//

#include <glm/gtc/type_ptr.hpp>
#include "shader.h"

Shader::Shader(const GLchar *vertex_path, const GLchar *frag_path) {
    //Get the code from the files
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    //error handling
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        //Open
        vShaderFile.open(vertex_path);
        fShaderFile.open(frag_path);
        std::stringstream vShaderStream, fShaderStream;

        //Read into stream
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();

        //Close
        vShaderFile.close();
        fShaderFile.close();

        //Convert to string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    } catch (std::ifstream::failure e) {
        std::cout << "File failed to read" << std::endl;
    }

    //Convert to usable format
    const GLchar *vShaderCode = vertexCode.c_str();
    const GLchar *fShaderCode = fragmentCode.c_str();

    //Create the shaders
    //Vertex shader
    GLuint vertex, fragment;
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);

    //Check if succuss
    int success;
    char info_log[512];
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, info_log);
        std::cout << "Error compiling vertex shader: " << info_log << std::endl;
    }

    //Fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);

    //Again check if success
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, info_log);
        std::cout << "Error compiling fragment shader: " << info_log << std::endl;
    }

    //Create the shader program of both the shaders
    program_id = glCreateProgram();
    glAttachShader(program_id, vertex);
    glAttachShader(program_id, fragment);
    glLinkProgram(program_id);

    //Check if success
    glGetShaderiv(program_id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(program_id, 512, NULL, info_log);
        std::cout << "Error linking program: " << info_log << std::endl;
    }

    //No longer needed, so delete
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::use() {
    glUseProgram(program_id);
}