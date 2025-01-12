#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <vector>
#include <memory>

namespace lark::physics::visualization {

class DronePhysicsRenderer {
public:
    DronePhysicsRenderer(int width = 1280, int height = 720) 
        : m_width(width), m_height(height) {
        initializeOpenGL();
        createShaders();
        createGeometry();
    }

    ~DronePhysicsRenderer() {
        cleanup();
    }

    bool shouldClose() const {
        return glfwWindowShouldClose(m_window);
    }

    void render() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Update view & projection matrices
        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 5.0f, 10.0f),  // Camera position
            glm::vec3(0.0f, 0.0f, 0.0f),   // Look at point
            glm::vec3(0.0f, 1.0f, 0.0f)    // Up vector
        );
        
        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            static_cast<float>(m_width) / static_cast<float>(m_height),
            0.1f,
            100.0f
        );

        // Use shader and set uniforms
        glUseProgram(m_shaderProgram);
        glUniformMatrix4fv(m_viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(m_projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Draw ground plane
        // Draw ground plane with proper scale and position
        glm::mat4 groundModel = glm::mat4(1.0f);
        groundModel = glm::translate(groundModel, glm::vec3(0.0f, -1.0f, 0.0f));
        groundModel = glm::scale(groundModel, glm::vec3(20.0f, 0.1f, 20.0f));
        glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, glm::value_ptr(groundModel));
        glUniform3f(m_colorLoc, 0.4f, 0.4f, 0.4f);  // Light gray for better visibility
        drawCube();

        // Draw test cube (will be replaced with drone components)
        glm::mat4 cubeModel = glm::mat4(1.0f);
        cubeModel = glm::translate(cubeModel, glm::vec3(0.0f, 2.0f, 0.0f));
        glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, glm::value_ptr(cubeModel));
        glUniform3f(m_colorLoc, 0.7f, 0.2f, 0.2f);  // Red color for cube
        drawCube();

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }

private:
    GLFWwindow* m_window;
    int m_width, m_height;
    GLuint m_shaderProgram;
    GLuint m_cubeVAO, m_cubeVBO;

    // Shader uniforms
    GLint m_modelLoc;
    GLint m_viewLoc;
    GLint m_projLoc;
    GLint m_colorLoc;

    void initializeOpenGL() {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        // Set OpenGL version based on platform
        #ifdef __APPLE__
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        #else
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        #endif

        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        m_window = glfwCreateWindow(m_width, m_height, "Drone Physics Test", nullptr, nullptr);
        if (!m_window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwMakeContextCurrent(m_window);
        glfwSwapInterval(1); // Enable vsync

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            throw std::runtime_error("Failed to initialize GLAD");
        }

        // Enable depth testing and back face culling
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);  // Darker background for better contrast
    }

    void createShaders() {
        const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 Normal;

        void main() {
            Normal = mat3(transpose(inverse(model))) * aNormal;
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;

        in vec3 Normal;
        uniform vec3 color;

        void main() {
            vec3 lightDir = normalize(vec3(0.0, 1.0, 0.0));  // Global directional light from above
            float diff = max(dot(normalize(Normal), lightDir), 0.0);
            float ambient = 0.3;
            vec3 result = (ambient + diff) * color;
            FragColor = vec4(result, 1.0);
        }
    )";

        // Create and compile vertex shader
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);

        // Create and compile fragment shader
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);

        // Create shader program
        m_shaderProgram = glCreateProgram();
        glAttachShader(m_shaderProgram, vertexShader);
        glAttachShader(m_shaderProgram, fragmentShader);
        glLinkProgram(m_shaderProgram);

        // Clean up shaders
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        // Get uniform locations
        m_modelLoc = glGetUniformLocation(m_shaderProgram, "model");
        m_viewLoc = glGetUniformLocation(m_shaderProgram, "view");
        m_projLoc = glGetUniformLocation(m_shaderProgram, "projection");
        m_colorLoc = glGetUniformLocation(m_shaderProgram, "color");
    }

    void createGeometry() {
        float vertices[] = {
            // Front face (+z)
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
             0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

            // Back face (-z)
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
             0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

            // Top face (+y)
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,

            // Bottom face (-y)
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
             0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
             0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,

            // Right face (+x)
             0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
             0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
             0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
             0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

            // Left face (-x)
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        };

        unsigned int indices[] = {
            0,  1,  2,  2,  3,  0,  // Front
            4,  5,  6,  6,  7,  4,  // Back
            8,  9,  10, 10, 11, 8,  // Top
            12, 13, 14, 14, 15, 12, // Bottom
            16, 17, 18, 18, 19, 16, // Right
            20, 21, 22, 22, 23, 20  // Left
        };

        glGenVertexArrays(1, &m_cubeVAO);
        glGenBuffers(1, &m_cubeVBO);
        glGenBuffers(1, &m_cubeEBO);

        glBindVertexArray(m_cubeVAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }), (void*)0);
        glEnableVertexAttribArray(0);

        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    void drawCube() {
        glBindVertexArray(m_cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);  // 6 faces * 2 triangles * 3 vertices = 36 vertices
    }

    void cleanup() {
        glDeleteVertexArrays(1, &m_cubeVAO);
        glDeleteBuffers(1, &m_cubeVBO);
        glDeleteProgram(m_shaderProgram);
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }
};

} // namespace lark::physics::visualization // namespace lark::physics::visualization