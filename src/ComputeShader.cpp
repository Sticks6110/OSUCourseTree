//
// Created by beast on 9/6/2026.
//

#include "ComputeShader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

ComputeShader::ComputeShader(const GLchar *shader_path) {
    //Get the code from the files
    std::string code;
    std::ifstream fileStream;

    //error handling
    fileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        //Open
        fileStream.open(shader_path);
        std::stringstream shaderStream;

        //Read into stream
        shaderStream << fileStream.rdbuf();

        //Close
        fileStream.close();

        //Convert to string
        code = shaderStream.str();
    } catch (std::ifstream::failure e) {
        std::cout << "File failed to read" << std::endl;
    }

    //Convert to usable format
    const GLchar *shaderCode = code.c_str();

    //Create the shader
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);

    glShaderSource(
        shader,
        1,
        &shaderCode,
        nullptr
    );

    glCompileShader(shader);

    //Check if succuss
    int success;
    char info_log[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        std::cout << "Error compiling compute shader: " << info_log << std::endl;
    }

    //Create the shader program
    program_id = glCreateProgram();

    glAttachShader(program_id, shader);
    glLinkProgram(program_id);

    //Check if success
    glGetProgramiv(program_id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program_id, 512, NULL, info_log);
        std::cout << "Error linking program: " << info_log << std::endl;
    }

    //No longer needed, so delete
    glDeleteShader(shader);
}

void ComputeShader::use() {
    glUseProgram(program_id);
}
