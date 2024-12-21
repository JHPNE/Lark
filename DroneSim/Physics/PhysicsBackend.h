#pragma once
#include <iostream>
#include <memory>
#include <string>
#include "glad/glad.h"
#include <glm/glm.hpp>
#include "PhysicsStructures.h"
#include "ShaderManager.h"

class PhysicsBackend {
  public:
    virtual ~PhysicsBackend() = default;

    virtual void updateRigidBodies(size_t count, float dt) = 0;

    virtual void detectCollisions(float dt) = 0;
    virtual void resolveCollisions(float dt) = 0;

    virtual Environment GetEnvironment() { return environment; }
    virtual std::string GetCompShader(const drosim::physics::shaders::compute_shaders type) { return drosim::physics::shaders::compShaders[type](); }

    static bool isGpuComputeSupported() {
      GLint major, minor;
      glGetIntegerv(GL_MAJOR_VERSION, &major);
      glGetIntegerv(GL_MINOR_VERSION, &minor);
      return (major > 4) || (major == 4 && minor >= 3);
    }

    virtual bool supportsGPUCollision() { return false; }

  private:
    Environment environment;
};