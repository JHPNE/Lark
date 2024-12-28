#include "ConstraintSystem.h"
#include "BodySystem.h"
#include <cmath>

namespace drosim::physics::cpu {
    void CreateDistanceConstraint(PhysicsWorld& world,
                                  uint32_t bodyA, uint32_t bodyB,
                                  const glm::vec3& localAnchorA,
                                  const glm::vec3& localAnchorB,
                                  float restLength)
    {
        ConstraintInfo c;
        c.bodyA = bodyA;
        c.bodyB = bodyB;
        c.localAnchorA = localAnchorA;
        c.localAnchorB = localAnchorB;
        c.restLength = restLength;
        world.constraints.push_back(c);
    }

    void SolveConstraints(PhysicsWorld& world, float dt, int iterations)
    {
        for (int iter = 0; iter < iterations; iter++) {
            // 1) Solve contact constraints
            for (auto& cp : world.contacts) {
                auto& bodyA = world.bodyPool[cp.bodyAIndex];
                auto& bodyB = world.bodyPool[cp.bodyBIndex];
                if (!bodyA.flags.active && !bodyB.flags.active) continue;

                glm::vec3 normal = cp.normal;
                float penetration = cp.penetration;

                glm::vec3 relPosA = cp.pointA - bodyA.motion.position;
                glm::vec3 relPosB = cp.pointB - bodyB.motion.position;

                glm::vec3 velA = bodyA.motion.velocity + glm::cross(bodyA.motion.angularVelocity, relPosA);
                glm::vec3 velB = bodyB.motion.velocity + glm::cross(bodyB.motion.angularVelocity, relPosB);
                glm::vec3 relVel = velB - velA;
                float normalVel = glm::dot(relVel, normal);

                float e = 0.5f * (bodyA.material.restitution + bodyB.material.restitution);
                float invMassA = bodyA.inertia.invMass;
                float invMassB = bodyB.inertia.invMass;

                glm::vec3 crossA = bodyA.inertia.globalInvInertia * glm::cross(relPosA, normal);
                glm::vec3 crossB = bodyB.inertia.globalInvInertia * glm::cross(relPosB, normal);

                float denom = invMassA + invMassB
                              + glm::dot(normal, glm::cross(crossA, relPosA))
                              + glm::dot(normal, glm::cross(crossB, relPosB));

                if (denom < 1e-9f) continue;

                // positional correction
                float baumgarte = 0.2f;
                float penetrationSlop = 0.01f;
                float penetrationDepth = std::max(0.f, penetration - penetrationSlop);
                float positionalBias = baumgarte * penetrationDepth / dt;

                float j = -(1.f + e) * normalVel + positionalBias;
                j /= denom;
                glm::vec3 impulse = j * normal;

                // apply impulses
                if (invMassA > 0.f && bodyA.flags.active) {
                    bodyA.motion.velocity -= impulse * invMassA;
                    bodyA.motion.angularVelocity -=
                        bodyA.inertia.globalInvInertia * glm::cross(relPosA, impulse);
                }
                if (invMassB > 0.f && bodyB.flags.active) {
                    bodyB.motion.velocity += impulse * invMassB;
                    bodyB.motion.angularVelocity +=
                        bodyB.inertia.globalInvInertia * glm::cross(relPosB, impulse);
                }

                // (Optional) friction, tangential impulses are missing here
            }

            // 2) Solve user constraints (distance)
            for (auto& c : world.constraints) {
                auto& bodyA = world.bodyPool[c.bodyA];
                auto& bodyB = world.bodyPool[c.bodyB];
                if (!bodyA.flags.active && !bodyB.flags.active) continue;

                glm::mat3 rotA = glm::mat3_cast(bodyA.motion.orientation);
                glm::mat3 rotB = glm::mat3_cast(bodyB.motion.orientation);
                glm::vec3 worldAnchorA = bodyA.motion.position + rotA * c.localAnchorA;
                glm::vec3 worldAnchorB = bodyB.motion.position + rotB * c.localAnchorB;

                glm::vec3 diff = worldAnchorB - worldAnchorA;
                float dist = glm::length(diff);
                if (dist < 1e-6f) continue;
                float error = dist - c.restLength;
                glm::vec3 dir = diff / dist;

                float invMassA = bodyA.inertia.invMass;
                float invMassB = bodyB.inertia.invMass;
                glm::vec3 rA = worldAnchorA - bodyA.motion.position;
                glm::vec3 rB = worldAnchorB - bodyB.motion.position;

                glm::vec3 crossA = bodyA.inertia.globalInvInertia * glm::cross(rA, dir);
                glm::vec3 crossB = bodyB.inertia.globalInvInertia * glm::cross(rB, dir);

                float denom = invMassA + invMassB
                            + glm::dot(dir, glm::cross(crossA, rA))
                            + glm::dot(dir, glm::cross(crossB, rB));

                if (denom < 1e-6f) continue;
                float lambda = -error / denom;
                glm::vec3 impulse = dir * lambda;

                // apply
                if (invMassA > 0.f && bodyA.flags.active) {
                    bodyA.motion.velocity -= impulse * invMassA;
                    bodyA.motion.angularVelocity -= bodyA.inertia.globalInvInertia
                                                    * glm::cross(rA, impulse);
                }
                if (invMassB > 0.f && bodyB.flags.active) {
                    bodyB.motion.velocity += impulse * invMassB;
                    bodyB.motion.angularVelocity += bodyB.inertia.globalInvInertia
                                                    * glm::cross(rB, impulse);
                }
            }
        }
    }
}
