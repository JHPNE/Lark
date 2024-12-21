#pragma once
#include "Physics/CpuPhysicsBackend.h"
#include "Physics/GpuPhysicsBackend.h"
#include <iostream>
#include <memory>
#include <string>
#include "GLFW/glfw3.h"

class PhysicsTests {
  public:
    void runTests() {
      rigidBodyTest();
    };
    void glInit() {
      if (!glfwInit())
      {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return;
      }

      // Change GLSL version based on platform
#ifdef __APPLE__
      const char* glsl_version = "#version 330";
#else
      const char* glsl_version = "#version 130";
#endif

      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
      glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
      glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
      glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GL_TRUE);
#endif


      //Creating a window
      GLFWwindow* m_window = glfwCreateWindow(1280, 720, "Native Editor", nullptr, nullptr);
      if (m_window == nullptr)
      {
        std::cerr << "Failed to create window" << std::endl;
        return;
      }

      glfwMakeContextCurrent(m_window);
      glfwSwapInterval(1); // Enable vsync

      // Initialize GLAD
      if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
      {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return;
      }

    }

    void rigidBodyTest() {
      glInit();
      printf("Starting RigidBody Tests\n");

      // Test configuration
      size_t count = 100;
      printf("Number of rigid bodies: %zu\n", count);

      RigidBodyArrays rbData;
      rbData.positions.resize(count, glm::vec3(0.0f));
      rbData.orientations.resize(count, glm::quat(1.0f,0.0f,0.0f,0.0f));
      rbData.linearVelocities.resize(count, glm::vec3(0.0f));
      rbData.angularVelocities.resize(count, glm::vec3(0.0f));
      rbData.massData.resize(count);
      rbData.inertiaData.resize(count);

      printf("Initializing rigid body data...\n");
      for (size_t i = 0; i < count; ++i) {
        rbData.positions[i] = glm::vec3((float)i, 0.0f, 0.0f);
        rbData.orientations[i] = glm::quat(1.0f,0.0f,0.0f,0.0f);
        rbData.linearVelocities[i] = glm::vec3(0.0f, (float)i*0.1f, 0.0f);
        rbData.angularVelocities[i] = glm::vec3((float)i*0.01f,0.0f,0.0f);

        float mass = 1.0f;
        float invMass = 1.0f/mass;
        glm::vec3 inertia(1.0f,1.0f,1.0f);
        glm::vec3 invInertia = 1.0f/inertia;

        rbData.massData[i] = glm::vec4(mass, invMass, inertia.x, inertia.y);
        rbData.inertiaData[i] = glm::vec4(inertia.z, invInertia.x, invInertia.y, invInertia.z);
      }

      printf("Finished initializing rigid bodies.\n");
      printf("Positions of first few bodies before simulation:\n");
      for (int i = 0; i < 5 && i < (int)count; ++i) {
        auto &pos = rbData.positions[i];
        printf("Body %d initial: Pos(%.3f, %.3f, %.3f), LinVel(%.3f, %.3f, %.3f)\n",
                i, pos.x, pos.y, pos.z,
                rbData.linearVelocities[i].x, rbData.linearVelocities[i].y, rbData.linearVelocities[i].z);
      }

      // Create backend
      std::unique_ptr<PhysicsBackend> backend;
      std::unique_ptr<PhysicsBackend> testBackend = std::make_unique<CpuPhysicsBackend>(rbData);
      if (PhysicsBackend::isGpuComputeSupported()) {
        printf("GPU compute is supported. Using GpuPhysicsBackend.\n");
        backend = std::make_unique<GpuPhysicsBackend>(rbData, count);
      } else {
        printf("GPU compute not supported. Using CpuPhysicsBackend.\n");
        backend = std::make_unique<CpuPhysicsBackend>(rbData);
      }

      printf("Starting simulation updates...\n");
      float dt = 0.016f; // 60 Hz
      for (int frame = 0; frame < 10; ++frame) {
        printf("Update frame %d with dt=%.4f...\n", frame, dt);

        // Perform the update
        backend->updateRigidBodies(count, dt);

        // Print out the position of a sample body after each update
        auto &samplePos = rbData.positions[0];
        auto &sampleOri = rbData.orientations[0];
        printf("After frame %d: Body 0 Pos(%.3f, %.3f, %.3f), Orientation(%.3f, %.3f, %.3f, %.3f)\n",
               frame, samplePos.x, samplePos.y, samplePos.z,
               sampleOri.w, sampleOri.x, sampleOri.y, sampleOri.z);
      }

      printf("Simulation complete. Printing final state of first 5 bodies:\n");
      for (int i = 0; i < 5 && i < (int)count; ++i) {
        auto &pos = rbData.positions[i];
        auto &q = rbData.orientations[i];
        std::cout << "Body " << i << ": Pos(" << pos.x << ", " << pos.y << ", " << pos.z
                  << ") Orientation(" << q.w << ", " << q.x << ", " << q.y << ", " << q.z << ")\n";
      }

      printf("RigidBody Tests completed.\n");
    }
};