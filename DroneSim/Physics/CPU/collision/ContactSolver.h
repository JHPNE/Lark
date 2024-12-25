#pragma once
#include "ContactInfo.h"
#include "../RigidBody.h"
#include <vector>

namespace drosim::physics {

// Stores constraint data for a single contact point
struct ContactConstraintPoint {
    ContactInfo contact;
    RigidBody* bodyA;
    RigidBody* bodyB;

    // Cached vectors from center of mass to contact point
    glm::vec3 rA;  // In world space
    glm::vec3 rB;  // In world space

    // Restitution and friction
    float restitution;
    float friction;

    // Cached constraint masses
    float normalMass;
    float tangentMass1;
    float tangentMass2;

    // Accumulated impulses for warm starting
    float normalImpulse;
    float tangentImpulse1;
    float tangentImpulse2;

    // Bias factor for position correction (Baumgarte)
    float bias;

    ContactConstraintPoint() :
        bodyA(nullptr),
        bodyB(nullptr),
        restitution(0.2f),
        friction(0.7f),
        normalMass(0.0f),
        tangentMass1(0.0f),
        tangentMass2(0.0f),
        normalImpulse(0.0f),
        tangentImpulse1(0.0f),
        tangentImpulse2(0.0f),
        bias(0.0f) {}
};

class ContactSolver {
public:
    void InitializeConstraints(const std::vector<ContactInfo>& contacts, 
                             float dt) {
        m_dt = dt;
        m_contacts.clear();
        m_contacts.reserve(contacts.size());

        for (const auto& contact : contacts) {
            ContactConstraintPoint point;
            point.contact = contact;
            
            // Get the bodies
            RigidBody* bodyA = contact.bodyA;  // Assuming ContactInfo stores body pointers
            RigidBody* bodyB = contact.bodyB;

            point.bodyA = bodyA;
            point.bodyB = bodyB;

            // Compute radii from center of mass to contact
            point.rA = contact.pointA - bodyA->GetPosition();
            point.rB = contact.pointB - bodyB->GetPosition();
            
            // Cache inverse masses
            float invMassA = bodyA->GetInverseMass();
            float invMassB = bodyB->GetInverseMass();
            
            // Get inverse inertia tensors in world space
            glm::mat3 invInertiaA = bodyA->GetLocalInverseInertiaTensor();
            glm::mat3 invInertiaB = bodyB->GetLocalInverseInertiaTensor();
            
            // Compute normal mass
            {
                glm::vec3 rnA = glm::cross(point.rA, contact.normal);
                glm::vec3 rnB = glm::cross(point.rB, contact.normal);
                
                glm::vec3 rotInertiaA = invInertiaA * rnA;
                glm::vec3 rotInertiaB = invInertiaB * rnB;
                
                float invMass = invMassA + invMassB;
                invMass += glm::dot(rnA, rotInertiaA);
                invMass += glm::dot(rnB, rotInertiaB);
                
                point.normalMass = invMass > 0.0f ? 1.0f / invMass : 0.0f;
            }
            
            // Compute tangent masses
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
            
            // Only apply restitution if closing velocity is significant
            if (normalVel < -1.0f) {
                point.bias = point.restitution * normalVel;
            }
            
            // Add Baumgarte position correction
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
            RigidBody* bodyA = point.bodyA;
            RigidBody* bodyB = point.bodyB;
            
            // Apply accumulated impulses
            glm::vec3 P = point.normalImpulse * point.contact.normal +
                         point.tangentImpulse1 * point.contact.tangent1 +
                         point.tangentImpulse2 * point.contact.tangent2;
            
            bodyA->ApplyImpulse(-P, point.rA);
            bodyB->ApplyImpulse(P, point.rB);
        }
    }

    void SolveVelocityConstraints() {
        for (auto& point : m_contacts) {
            RigidBody* bodyA = point.bodyA;
            RigidBody* bodyB = point.bodyB;
            
            // Get velocities
            glm::vec3 vA = bodyA->GetVelocity();
            glm::vec3 wA = bodyA->GetAngularVelocity();
            glm::vec3 vB = bodyB->GetVelocity();
            glm::vec3 wB = bodyB->GetAngularVelocity();
            
            // Relative velocity at contact
            glm::vec3 relativeVel = (vB + glm::cross(wB, point.rB)) -
                                   (vA + glm::cross(wA, point.rA));
            
            // Normal constraint
            {
                float normalVel = glm::dot(relativeVel, point.contact.normal);
                float lambda = -point.normalMass * (normalVel + point.bias);
                
                // Accumulate impulses (clamped to prevent separation)
                float oldImpulse = point.normalImpulse;
                point.normalImpulse = glm::max(oldImpulse + lambda, 0.0f);
                lambda = point.normalImpulse - oldImpulse;
                
                glm::vec3 P = lambda * point.contact.normal;
                bodyA->ApplyImpulse(-P, point.rA);
                bodyB->ApplyImpulse(P, point.rB);
            }
            
            // Friction constraints
            {
                // Update relative velocity
                relativeVel = (vB + glm::cross(wB, point.rB)) -
                             (vA + glm::cross(wA, point.rA));
                
                // Tangent forces
                float maxFriction = point.friction * point.normalImpulse;
                
                // First tangent direction
                float lambda1 = -point.tangentMass1 * 
                               glm::dot(relativeVel, point.contact.tangent1);
                float oldImpulse1 = point.tangentImpulse1;
                point.tangentImpulse1 = glm::clamp(oldImpulse1 + lambda1,
                                                  -maxFriction, maxFriction);
                lambda1 = point.tangentImpulse1 - oldImpulse1;
                
                glm::vec3 P1 = lambda1 * point.contact.tangent1;
                bodyA->ApplyImpulse(-P1, point.rA);
                bodyB->ApplyImpulse(P1, point.rB);
                
                // Second tangent direction
                float lambda2 = -point.tangentMass2 * 
                               glm::dot(relativeVel, point.contact.tangent2);
                float oldImpulse2 = point.tangentImpulse2;
                point.tangentImpulse2 = glm::clamp(oldImpulse2 + lambda2,
                                                  -maxFriction, maxFriction);
                lambda2 = point.tangentImpulse2 - oldImpulse2;
                
                glm::vec3 P2 = lambda2 * point.contact.tangent2;
                bodyA->ApplyImpulse(-P2, point.rA);
                bodyB->ApplyImpulse(P2, point.rB);
            }
        }
    }

    void Solve() {
        // Typically use multiple iterations for stability
        const int velocityIterations = 8;
        
        for (int i = 0; i < velocityIterations; ++i) {
            SolveVelocityConstraints();
        }
    }

private:
    std::vector<ContactConstraintPoint> m_contacts;
    float m_dt;
};

} // namespace drosim::physics