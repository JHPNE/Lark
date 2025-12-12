#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "glad/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class ShaderParser
{

public:
    static std::string LoadShaderSource(const std::string& filepath)
    {
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            std::cerr << "Failed to open shader file: " << filepath << std::endl;
            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    static GLuint CompileShader(GLenum type, const std::string& src)
    {
        GLuint shader = glCreateShader(type);
        const char* c_str = src.c_str();
        glShaderSource(shader, 1, &c_str, nullptr);
        glCompileShader(shader);

        // Check for errors
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[512];
            glGetShaderInfoLog(shader, 512, nullptr, log);
            std::cerr << "Shader compilation failed:\n" << log << std::endl;
            glDeleteShader(shader);
        }

        return shader;
    }

    static GLuint CreateShaderProgram(const std::string& vertexPath,
                           const std::string& fragmentPath)
    {
        std::string vertexSrc   = LoadShaderSource(vertexPath);
        std::string fragmentSrc = LoadShaderSource(fragmentPath);

        // if (vertexSrc.empty() || fragmentSrc.empty()) {};

        GLuint vertexShader   = CompileShader(GL_VERTEX_SHADER, vertexSrc);
        GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);

        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        // Check for linking errors
        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success)
        {
            char log[512];
            glGetProgramInfoLog(program, 512, nullptr, log);
            std::cerr << "Program linking failed:\n" << log << std::endl;
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            glDeleteProgram(program);
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return program;
    }
};



