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
    // I want SOC too much in one file
    static std::string ProcessIncludes(const std::string& source,
                                      const std::string& directory,
                                      std::unordered_set<std::string>& includedFiles)
    {
        std::stringstream result;
        std::istringstream stream(source);
        std::string line;
        int lineNumber = 1;

        while (std::getline(stream, line))
        {
            size_t includePos = line.find("#include");
            if (includePos != std::string::npos)
            {
                size_t firstQuote = line.find('"', includePos);
                size_t secondQuote = line.find('"', firstQuote + 1);

                if (firstQuote != std::string::npos && secondQuote != std::string::npos)
                {
                    std::string filename = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
                    std::string includePath = directory + "/" + filename;

                    if (includedFiles.find(includePath) != includedFiles.end())
                    {
                        std::cerr << "Warning: Circular include detected: " << includePath << std::endl;
                        continue;
                    }

                    includedFiles.insert(includePath);

                    std::string includeContent = LoadShaderSourceRaw(includePath);
                    if (!includeContent.empty())
                    {
                        result << "// Begin include: " << filename << "\n";
                        result << ProcessIncludes(includeContent, directory, includedFiles);
                        result << "// End include: " << filename << "\n";
                    }
                    else
                    {
                        std::cerr << "Failed to include file: " << includePath << std::endl;
                    }
                }
                else
                {
                    std::cerr << "Invalid #include syntax at line " << lineNumber << std::endl;
                }
            }
            else
            {
                result << line << "\n";
            }

            lineNumber++;
        }

        return result.str();
    }

    static std::string LoadShaderSourceRaw(const std::string& filepath)
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

public:
    static std::string LoadShaderSource(const std::string& filepath)
    {
        std::string source = LoadShaderSourceRaw(filepath);
        if (source.empty())
        {
            return "";
        }

        std::filesystem::path path(filepath);
        std::string directory = path.parent_path().string();

        std::unordered_set<std::string> includedFiles;
        includedFiles.insert(filepath);

        return ProcessIncludes(source, directory, includedFiles);
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



