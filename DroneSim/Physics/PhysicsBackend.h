#pragma once
#include <iostream>
#include <memory>
#include <cmath>
#include <chrono>
#include <string>
#include "glad/glad.h"
#include "PhysicsStructures.h"
#include "ShaderManager.h"

using namespace drosim::physics::shaders;

class PhysicsBackend {
  public:
    virtual ~PhysicsBackend() {}

    virtual void updateRigidBodies(size_t count, float dt) = 0;

    virtual void detectCollisions(float dt) = 0;
    virtual void resolveCollisions(float dt) = 0;

    virtual Environment GetEnvironment() { return environment; }
    virtual std::string GetCompShader(physics_type type) { return compShaders[type](); }

    bool isGpuComputeSupported() {
      GLint major, minor;
      glGetIntegerv(GL_MAJOR_VERSION, &major);
      glGetIntegerv(GL_MINOR_VERSION, &minor);
      return (major > 4) || (major == 4 && minor >= 3);
    }

    virtual bool supportsGPUCollision() { return false; }

  private:
    Environment environment;
};