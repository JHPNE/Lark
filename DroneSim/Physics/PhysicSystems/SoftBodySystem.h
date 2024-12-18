#pragma once
#include "../IPhysicsSystem.h"

namespace drosim::physics::softbody {
  struct SoftBody {
    float mass;
    std::vector<math::v3> vertices;
    std::vector<std::pair<size_t, size_t>> constraints;
  };

  using SoftBodyID = uint32_t;

  class SoftBodySystem final : public IPhysicsSystem {
    public:
      SoftBodySystem() = default;
      ~SoftBodySystem() = default;

      void Initialize() override {
        softBodies.reserve(1000); // Preallocate to avoid frequent reallocations
      }

      void Update(float deltaTime) override {
        // Integrate soft body physics
        for(auto& sb : softBodies) {
          // Update vertices based on forces and constraints
          // Simplified example
          for(auto& vertex : sb.vertices) {
            // Apply gravity or other forces
            vertex.y += -9.81f * deltaTime;
          }
        }
      }

    private:
      std::vector<SoftBody> softBodies;
  };
};