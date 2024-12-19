#pragma once
#include <string>
#include <filesystem>

namespace drosim::physics {

  namespace shaders {
    using shader_creator = std::string(*)();

    std::string rigidBody();

    std::string rigidBody() {
      // Use std::filesystem to get the directory of the current file and append the shader path.
      std::filesystem::path currentFilePath = __FILE__;
      std::filesystem::path shaderPath = currentFilePath.parent_path() / "Shaders/rigidbody_comp.glsl";

      return std::filesystem::absolute(shaderPath).string();
    };

    enum physics_type {
      rigid_body,
    };

    shader_creator compShaders[] {
      rigidBody,
    };
  }
}