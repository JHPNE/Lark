// ContactSolver.h
#pragma once
#include "ContactInfo.h"
#include "../RigidBody.h"
#include <vector>
#include <algorithm>

namespace drosim::physics {

struct ContactConstraintPoint {
    ContactInfo contact;
    RigidBody* bodyA = nullptr;
    RigidBody* bodyB = nullptr;

    // Cached vectors from center of mass to contact point
    glm::vec3 rA = glm::vec3(0.0f);  // In world space
    glm::vec3 rB = glm::vec3(0.0f);  // In world space

    // Restitution and friction
    float restitution = 0.2f;
    float friction = 0.7f;

    // Cached constraint masses
    float normalMass = 0.0f;
    float tangentMass1 = 0.0f;
    float tangentMass2 = 0.0f;

    // Accumulated impulses for warm starting
    float normalImpulse = 0.0f;
    float tangentImpulse1 = 0.0f;
    float tangentImpulse2 = 0.0f;

    // Bias factor for position correction
    float bias = 0.0f;

    // Safety threshold
    static constexpr float MAX_IMPULSE = 1000.0f;
};

class ContactSolver {
public:
    void InitializeConstraints(const std::vector<ContactInfo>& contacts, float dt) {
        m_dt = dt;
        m_contacts.clear();
        m_contacts.reserve(contacts.size());

        for (const auto& contact : contacts) {
            if (!ValidateContact(contact)) {
                continue;
            }

            ContactConstraintPoint point;
            point.contact = contact;

            RigidBody* bodyA = contact.bodyA;
            RigidBody* bodyB = contact.bodyB;

            // Validate bodies
            if (!bodyA || !bodyB) {
                continue;
            }

            point.bodyA = bodyA;
            point.bodyB = bodyB;

            // Compute contact points relative to COM
            point.rA = contact.pointA - bodyA->GetPosition();
            point.rB = contact.pointB - bodyB->GetPosition();

            // Get inverse masses and validate
            float invMassA = bodyA->GetInverseMass();
            float invMassB = bodyB->GetInverseMass();

            if (invMassA < 0.0f || invMassB < 0.0f) {
                continue;
            }

            // Get inverse inertia tensors
            const glm::mat3& invInertiaA = bodyA->GetGlobalInverseInertiaTensor();
            const glm::mat3& invInertiaB = bodyB->GetGlobalInverseInertiaTensor();

            // Compute effective mass for normal constraint
            {
                glm::vec3 rnA = glm::cross(point.rA, contact.normal);
                glm::vec3 rnB = glm::cross(point.rB, contact.normal);

                float invMass = invMassA + invMassB;
                invMass += glm::dot(rnA, invInertiaA * rnA);
                invMass += glm::dot(rnB, invInertiaB * rnB);

                point.normalMass = invMass > 0.0f ? 1.0f / invMass : 0.0f;
            }

            // Compute effective mass for friction constraints
            {
                glm::vec3 rt1A = glm::cross(point.rA, contact.tangent1);
                glm::vec3 rt1B = glm::cross(point.rB, contact.tangent1);
                glm::vec3 rt2A = glm::cross(point.rA, contact.tangent2);
                glm::vec3 rt2B = glm::cross(point.rB, contact.tangent2);

                float invMass1 = invMassA + invMassB;
                invMass1 += glm::dot(rt1A, invInertiaA * rt1A);
                invMass1 += glm::dot(rt1B, invInertiaB * rt1B);

                float invMass2 = invMassA + invMassB;
                invMass2 += glm::dot(rt2A, invInertiaA * rt2A);
                invMass2 += glm::dot(rt2B, invInertiaB * rt2B);

                point.tangentMass1 = invMass1 > 0.0f ? 1.0f / invMass1 : 0.0f;
                point.tangentMass2 = invMass2 > 0.0f ? 1.0f / invMass2 : 0.0f;
            }

            // Compute restitution bias
            glm::vec3 vA = bodyA->GetVelocity() +
                          glm::cross(bodyA->GetAngularVelocity(), point.rA);
            glm::vec3 vB = bodyB->GetVelocity() +
                          glm::cross(bodyB->GetAngularVelocity(), point.rB);
            glm::vec3 relativeVel = vB - vA;

            float normalVel = glm::dot(relativeVel, contact.normal);

            // Only apply restitution for significant velocities
            const float RESTITUTION_THRESHOLD = -1.0f;
            if (normalVel < RESTITUTION_THRESHOLD) {
                point.bias = point.restitution * normalVel;
            }

            // Add Baumgarte stabilization
            const float BAUMGARTE_FACTOR = 0.2f;
            const float PENETRATION_SLOP = 0.005f;
            float penetration = contact.penetrationDepth - PENETRATION_SLOP;

            if (penetration > 0.0f) {
                point.bias += (BAUMGARTE_FACTOR / m_dt) * penetration;
            }

            m_contacts.push_back(point);
        }
    }

    void WarmStart() {
        for (auto& point : m_contacts) {
            if (!point.bodyA || !point.bodyB) continue;

            // Compute total impulse from accumulated values
            glm::vec3 P = point.normalImpulse * point.contact.normal +
                         point.tangentImpulse1 * point.contact.tangent1 +
                         point.tangentImpulse2 * point.contact.tangent2;

            // Clamp impulse magnitude for stability
            float magnitude = glm::length(P);
            if (magnitude > ContactConstraintPoint::MAX_IMPULSE) {
                P *= ContactConstraintPoint::MAX_IMPULSE / magnitude;
            }

            // Apply impulses
            SafelyApplyImpulses(point.bodyA, point.bodyB, P, point.rA, point.rB);
        }
    }

    void Solve() {
        const int VELOCITY_ITERATIONS = 8;
        const int MIN_ITERATIONS = 4;

        for (int i = 0; i < VELOCITY_ITERATIONS; ++i) {
            bool stillMoving = SolveVelocityConstraints();

            // Early out if system stabilizes, but ensure minimum iterations
            if (!stillMoving && i >= MIN_ITERATIONS) {
                break;
            }
        }
    }

private:
    bool ValidateContact(const ContactInfo& contact) {
        if (!contact.bodyA || !contact.bodyB) return false;

        // Check for valid normal
        float normalLength = glm::length(contact.normal);
        if (normalLength < 0.999f || normalLength > 1.001f) return false;

        // Check for valid tangents
        if (glm::length(contact.tangent1) < 0.999f ||
            glm::length(contact.tangent2) < 0.999f) return false;

        // Verify penetration depth
        if (std::isnan(contact.penetrationDepth) ||
            std::isinf(contact.penetrationDepth)) return false;

        return true;
    }

    void SafelyApplyImpulses(RigidBody* bodyA, RigidBody* bodyB,
                            const glm::vec3& P,
                            const glm::vec3& rA, const glm::vec3& rB) {
        if (!bodyA || !bodyB) return;

        bodyA->ApplyImpulse(-P, bodyA->GetPosition() + rA);
        bodyB->ApplyImpulse(P, bodyB->GetPosition() + rB);
    }

    bool SolveVelocityConstraints() {
        bool anySignificantMotion = false;
        const float MOTION_THRESHOLD = 0.01f;

        for (auto& point : m_contacts) {
            if (!point.bodyA || !point.bodyB) continue;

            // Get current velocities
            glm::vec3 vA = point.bodyA->GetVelocity();
            glm::vec3 wA = point.bodyA->GetAngularVelocity();
            glm::vec3 vB = point.bodyB->GetVelocity();
            glm::vec3 wB = point.bodyB->GetAngularVelocity();

            // Relative velocity at contact
            glm::vec3 relativeVel = (vB + glm::cross(wB, point.rB)) -
                                   (vA + glm::cross(wA, point.rA));

            // Normal constraint
            {
                float normalVel = glm::dot(relativeVel, point.contact.normal);
                float lambda = -point.normalMass * (normalVel + point.bias);

                // Clamp accumulated impulse
                float oldImpulse = point.normalImpulse;
                point.normalImpulse = std::max(oldImpulse + lambda, 0.0f);
                lambda = point.normalImpulse - oldImpulse;

                glm::vec3 P = lambda * point.contact.normal;
                SafelyApplyImpulses(point.bodyA, point.bodyB, P, point.rA, point.rB);

                if (std::abs(lambda) > MOTION_THRESHOLD) {
                    anySignificantMotion = true;
                }
            }

            // Update velocities after normal constraint
            vA = point.bodyA->GetVelocity();
            wA = point.bodyA->GetAngularVelocity();
            vB = point.bodyB->GetVelocity();
            wB = point.bodyB->GetAngularVelocity();
            relativeVel = (vB + glm::cross(wB, point.rB)) -
                         (vA + glm::cross(wA, point.rA));

            // Friction constraints
            float maxFriction = point.friction * point.normalImpulse;

            // First friction direction
            {
                float lambda1 = -point.tangentMass1 *
                               glm::dot(relativeVel, point.contact.tangent1);
                float oldImpulse = point.tangentImpulse1;
                point.tangentImpulse1 = glm::clamp(oldImpulse + lambda1,
                                                  -maxFriction, maxFriction);
                lambda1 = point.tangentImpulse1 - oldImpulse;

                glm::vec3 P = lambda1 * point.contact.tangent1;
                SafelyApplyImpulses(point.bodyA, point.bodyB, P, point.rA, point.rB);

                if (std::abs(lambda1) > MOTION_THRESHOLD) {
                    anySignificantMotion = true;
                }
            }

            // Update velocities after first friction constraint
            vA = point.bodyA->GetVelocity();
            wA = point.bodyA->GetAngularVelocity();
            vB = point.bodyB->GetVelocity();
            wB = point.bodyB->GetAngularVelocity();
            relativeVel = (vB + glm::cross(wB, point.rB)) -
                         (vA + glm::cross(wA, point.rA));

            // Second friction direction
            {
                float lambda2 = -point.tangentMass2 *
                               glm::dot(relativeVel, point.contact.tangent2);
                float oldImpulse = point.tangentImpulse2;
                point.tangentImpulse2 = glm::clamp(oldImpulse + lambda2,
                                                  -maxFriction, maxFriction);
                lambda2 = point.tangentImpulse2 - oldImpulse;

                glm::vec3 P = lambda2 * point.contact.tangent2;
                SafelyApplyImpulses(point.bodyA, point.bodyB, P, point.rA, point.rB);

                if (std::abs(lambda2) > MOTION_THRESHOLD) {
                    anySignificantMotion = true;
                }
            }
        }

        return anySignificantMotion;
    }

    std::vector<ContactConstraintPoint> m_contacts;
    float m_dt = 0.0f;
};

} // namespace drosim::physics