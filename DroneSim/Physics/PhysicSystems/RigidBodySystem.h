#pragma once
#include "../IPhysicsSystem.h"
#include <vector>
#include <cstdint>

namespace drosim::physics::rigidbody {

    struct RigidBody {
        float mass;
        math::v3 position;
        math::v3 velocity;
        math::v3 acceleration;
        math::v3 forcesAccum;
        float boundingSphereRadius;
        bool isStatic;
    };

    using RigidBodyID = uint32_t;

    class RigidBodySystem final : public IPhysicsSystem {
    public:
        RigidBodySystem() = default;
        ~RigidBodySystem() = default;

        void Initialize() override {
            rigidBodies.reserve(1000); // Preallocate to avoid frequent reallocations
        }

        void Update(float deltaTime) override {
            for (auto &rb : rigidBodies) {
                if (!rb.isStatic) {
                    // Simple Euler Integration
                    rb.acceleration = rb.forcesAccum / rb.mass;
                    rb.velocity += rb.acceleration * deltaTime;
                    rb.position += rb.velocity * deltaTime;

                    // Reset accumulated forces
                    rb.forcesAccum = math::v3{0.0f, 0.0f, 0.0f};
                }
            }
        }

        RigidBodyID CreateRigidBody(float mass, const math::v3& position, const math::v3& velocity,
                                    const math::v3& acceleration, const math::v3& forces,
                                    float boundingSphereRadius, bool isStatic = false) {
            rigidBodies.emplace_back(RigidBody{mass, position, velocity, acceleration, forces, boundingSphereRadius, isStatic});
            return static_cast<RigidBodyID>(rigidBodies.size() - 1);
        }

        void ApplyForce(RigidBodyID id, const math::v3& force) {
            if(id < rigidBodies.size()) {
                rigidBodies[id].forcesAccum += force;
            }
            // Optionally handle invalid ID cases
        }

        const std::vector<RigidBody>& GetRigidBodies() const {
            return rigidBodies;
        }

    private:
        std::vector<RigidBody> rigidBodies;
    };

}
