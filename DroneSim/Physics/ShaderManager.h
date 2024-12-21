#pragma once
#include <string>
#include <filesystem>



namespace drosim::physics::shaders {
  using shader_creator = std::string (*)();
  inline std::filesystem::path currentFilePath = __FILE__;

  std::string physicsUpdate();
  std::string mortonCodes();
  std::string radixSort();
  std::string buildLBVH();
  std::string refitLBVH();
  std::string collisionDetection();

  // basically rigidbody
  inline std::string physicsUpdate() {
    const std::filesystem::path shaderPath = currentFilePath.parent_path() / "Shaders/PhysicsUpdate.comp";
    return absolute(shaderPath).string();
  };

  inline std::string mortonCodes() {
    const std::filesystem::path shaderPath =
        currentFilePath.parent_path() / "Shaders/ComputeMortonCodes.comp";
    return absolute(shaderPath).string();
  }

  inline std::string radixSort() {
    const std::filesystem::path shaderPath =
        currentFilePath.parent_path() / "Shaders/RadixSort.comp";
    return absolute(shaderPath).string();
  }

  inline std::string buildLBVH() {
    const std::filesystem::path shaderPath =
        currentFilePath.parent_path() / "Shaders/BuildLBVH.comp";
    return absolute(shaderPath).string();
  }

  inline std::string refitLBVH() {
    const std::filesystem::path shaderPath =
        currentFilePath.parent_path() / "Shaders/RefitBVH.comp";
    return absolute(shaderPath).string();
  }

  inline std::string collisionDetection() {
    const std::filesystem::path shaderPath = currentFilePath.parent_path() / "Shaders/CollisionDetection.comp";
    return absolute(shaderPath).string();
  }

  enum compute_shaders {
    physics_update,
    morton_codes,
    radix_sort,
    build_lbvh,
    refit_bvh,
    collision_detection
  };

  inline shader_creator compShaders[] {
    physicsUpdate,
    mortonCodes,
    radixSort,
    buildLBVH,
    refitLBVH,
    collisionDetection
  };
}
