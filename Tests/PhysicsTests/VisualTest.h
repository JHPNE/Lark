#pragma once
#include "Physics/CPU-Compute/PhysicsData.h"
#include <glad/glad.h>
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vector>

namespace drosim::physics::visualization {

class SimplePhysicsRenderer {
public:
    SimplePhysicsRenderer(int width = 800, int height = 600)
        : m_width(width), m_height(height) {
        initWindow();
        initGL();
        createShaders();
        createGeometry();
    }

    ~SimplePhysicsRenderer() {
        glDeleteProgram(shaderProgram);
        glDeleteBuffers(1, &circleVBO);
        glDeleteVertexArrays(1, &circleVAO);
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    bool shouldClose() {
        return glfwWindowShouldClose(m_window);
    }

    void render(const cpu::PhysicsWorld& world) {
        glClear(GL_COLOR_BUFFER_BIT);

        // Use shader program
        glUseProgram(shaderProgram);

        // Set transform uniforms
        float scale = 50.0f;  // 1 physics unit = 50 pixels
        float offsetX = m_width / 2.0f;
        float offsetY = m_height / 4.0f;

        // Update viewport and projection matrix for window size
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);
        glViewport(0, 0, width, height);

        float projection[16] = {
            2.0f/width, 0.0f, 0.0f, -1.0f,
            0.0f, 2.0f/height, 0.0f, -1.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };

        GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);

        // Draw boxes
        for (size_t i = 0; i < world.boxPool.Size(); i++) {
            const auto& box = world.boxPool[i];
            const auto& body = world.bodyPool[box.bodyIndex];

            float x = body.motion.position.x * scale + offsetX;
            float y = body.motion.position.y * scale + offsetY;
            float width = box.halfExtents.x * 2 * scale;
            float height = box.halfExtents.y * 2 * scale;

            // Set color (gray for boxes)
            GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
            float color[] = {0.5f, 0.5f, 0.5f, 1.0f};
            glUniform4fv(colorLoc, 1, color);

            // Set transform
            float transform[16] = {
                width, 0.0f, 0.0f, x,
                0.0f, height, 0.0f, y,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            };
            GLint transformLoc = glGetUniformLocation(shaderProgram, "transform");
            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, transform);

            // Draw box
            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }

        // Draw spheres
        for (size_t i = 0; i < world.spherePool.Size(); i++) {
            const auto& sphere = world.spherePool[i];
            const auto& body = world.bodyPool[sphere.bodyIndex];

            float x = body.motion.position.x * scale + offsetX;
            float y = body.motion.position.y * scale + offsetY;
            float radius = sphere.radius * scale;

            // Set color (red for spheres)
            GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
            float color[] = {1.0f, 0.0f, 0.0f, 1.0f};
            glUniform4fv(colorLoc, 1, color);

            // Set transform
            float transform[16] = {
                radius, 0.0f, 0.0f, x,
                0.0f, radius, 0.0f, y,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            };
            GLint transformLoc = glGetUniformLocation(shaderProgram, "transform");
            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, transform);

            // Draw circle
            glBindVertexArray(circleVAO);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 33); // 32 segments + center point
        }

        // Draw constraints
        for (const auto& constraint : world.constraints) {
            const auto& bodyA = world.bodyPool[constraint.bodyA];
            const auto& bodyB = world.bodyPool[constraint.bodyB];

            float x1 = bodyA.motion.position.x * scale + offsetX;
            float y1 = bodyA.motion.position.y * scale + offsetY;
            float x2 = bodyB.motion.position.x * scale + offsetX;
            float y2 = bodyB.motion.position.y * scale + offsetY;

            // Set color (green for constraints)
            GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
            float color[] = {0.0f, 1.0f, 0.0f, 1.0f};
            glUniform4fv(colorLoc, 1, color);

            // Update line vertices
            float lineVertices[] = {x1, y1, x2, y2};
            glBindVertexArray(lineVAO);
            glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lineVertices), lineVertices);
            glDrawArrays(GL_LINES, 0, 2);
        }

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }

private:
    GLFWwindow* m_window;
    int m_width, m_height;
    GLuint shaderProgram;
    GLuint circleVAO, circleVBO;
    GLuint quadVAO, quadVBO;
    GLuint lineVAO, lineVBO;

    void initWindow() {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

#ifdef __APPLE__
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        m_window = glfwCreateWindow(m_width, m_height, "Physics Simulation", nullptr, nullptr);
        if (!m_window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwMakeContextCurrent(m_window);
        glfwSwapInterval(1); // Enable vsync
    }

    void initGL() {
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            throw std::runtime_error("Failed to initialize GLAD");
        }

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    }

    void createShaders() {
        const char* vertexShaderSource = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            uniform mat4 projection;
            uniform mat4 transform;
            void main() {
                gl_Position = projection * transform * vec4(aPos, 0.0, 1.0);
            }
        )";

        const char* fragmentShaderSource = R"(
            #version 330 core
            uniform vec4 color;
            out vec4 FragColor;
            void main() {
                FragColor = color;
            }
        )";

        // Create vertex shader
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);

        // Create fragment shader
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);

        // Create shader program
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    void createGeometry() {
        // Create circle geometry
        std::vector<float> circleVertices;
        circleVertices.push_back(0.0f); // Center point
        circleVertices.push_back(0.0f);

        const int segments = 32;
        for (int i = 0; i <= segments; i++) {
            float angle = i * 2.0f * 3.14159f / segments;
            circleVertices.push_back(cos(angle));
            circleVertices.push_back(sin(angle));
        }

        glGenVertexArrays(1, &circleVAO);
        glGenBuffers(1, &circleVBO);

        glBindVertexArray(circleVAO);
        glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
        glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float),
                    circleVertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Create quad geometry
        float quadVertices[] = {
            -0.5f, -0.5f,
             0.5f, -0.5f,
             0.5f,  0.5f,
            -0.5f,  0.5f
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);

        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Create line geometry
        float lineVertices[] = {
            0.0f, 0.0f,
            1.0f, 1.0f
        };

        glGenVertexArrays(1, &lineVAO);
        glGenBuffers(1, &lineVBO);

        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }
};

} // namespace