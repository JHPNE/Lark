#include "PhysicsWorld.h"

#include "Colliders/BoxCollider.h"

#include <iostream>

namespace drosim::physics {

  std::shared_ptr<RigidBody> PhysicsWorld::CreateRigidBody() {
    auto body = std::make_shared<RigidBody>();
    m_rigidBodies.push_back(body);
    return body;
  }

void PhysicsWorld::UpdateRigidBodyAABBs() {
    // For each rigid body,
    for (const auto& body : m_rigidBodies) {
      // For each Collider in that body
      for (auto& colliderPtr : body->GetColliders()) {
        // Check if it’s a BoxCollider (or you can do Sphere, etc.)
        if (auto* boxCollider = dynamic_cast<BoxCollider*>(colliderPtr.get())) {
          AABB* aabb = boxCollider->GetAABB();
          if (!aabb) continue;

          // Debug old bounds
          glm::vec3 oldMin = aabb->minPoint;
          glm::vec3 oldMax = aabb->maxPoint;

          // Update the AABB
          boxCollider->UpdateAABBBounds();

          // Optional: print debug info
          std::cout << "AABB for body at Y=" << body->GetPosition().y
                    << " moved from [" << oldMin.y << ", " << oldMax.y
                    << "] to [" << aabb->minPoint.y << ", " << aabb->maxPoint.y << "]\n";
        }
      }
    }

    // Finally, tell the broadphase to update its structure
    if (m_broadphase) {
      m_broadphase->Update();
    }
  }


  void PhysicsWorld::StepSimulation(float dt) {

    // 1. Integrate each body
    for (auto& rigidBody : m_rigidBodies) {
      if (rigidBody->GetInverseMass() == 0.0f) {
        continue;
      }

      // For simplicity, apply gravity to dynamic bodies

      glm::vec3 force(0.0f, -9.81f * rigidBody->GetMass(), 0.0f);
      rigidBody->ApplyForce(force, rigidBody->GetPosition());
      rigidBody->Integrate(dt);
    }

    // 2. Update AABBs in the broadphase
    UpdateRigidBodyAABBs();

    // 3. Narrowphase collision detection
    const auto& potentialPairs = m_broadphase->ComputePairs();
    std::vector<ContactInfo> contacts;
    contacts.reserve(potentialPairs.size());

    for (const auto& pair : potentialPairs) {
      Collider* colliderA = pair.first;
      Collider* colliderB = pair.second;

      // Skip static-static
      if (colliderA->GetRigidBody()->GetInverseMass() == 0.0f &&
          colliderB->GetRigidBody()->GetInverseMass() == 0.0f) {
        continue;
          }

      ContactInfo contact;
      if (GJKAlgorithm::DetectCollision(colliderA, colliderB, contact)) {
        contacts.push_back(contact);
      }
    }

    // 5. Collision resolution
    const int velocityIterations = 8;
    const float beta = 0.2f; // Baumgarte factor
    const float slop = 0.005f; // Penetration slop



    for (int i = 0; i < velocityIterations; ++i) {
      for (auto& contact : contacts) {
        RigidBody* bodyA = contact.bodyA;
        RigidBody* bodyB = contact.bodyB;

        // Get points relative to centers of mass
        glm::vec3 rA = contact.pointA - bodyA->GetPosition();
        glm::vec3 rB = contact.pointB - bodyB->GetPosition();

        // Compute relative velocity
        glm::vec3 relativeVel = (bodyB->GetVelocity() + glm::cross(bodyB->GetAngularVelocity(), rB)) -
                               (bodyA->GetVelocity() + glm::cross(bodyA->GetAngularVelocity(), rA));

        // Normal impulse
        float normalVel = glm::dot(relativeVel, contact.normal);

        // Restitution
        float restitution = std::min(bodyA->GetRestitution(), bodyB->GetRestitution());

        // Position correction
        float penetration = std::max(contact.penetrationDepth - slop, 0.0f);
        float bias = (beta / dt) * penetration;

        if (normalVel < -1.0f) {
          bias += restitution * normalVel;
        }

        // Compute normal mass
        glm::vec3 crossA = glm::cross(rA, contact.normal);
        glm::vec3 crossB = glm::cross(rB, contact.normal);

        float invMassSum = bodyA->GetInverseMass() + bodyB->GetInverseMass() +
                         glm::dot(crossA, bodyA->GetGlobalInverseInertiaTensor() * crossA) +
                         glm::dot(crossB, bodyB->GetGlobalInverseInertiaTensor() * crossB);

        float normalImpulse = -(normalVel + bias) / invMassSum;

        // Apply normal impulse
        glm::vec3 P = contact.normal * normalImpulse;
        bodyA->ApplyImpulse(-P, rA);
        bodyB->ApplyImpulse(P, rB);

        // Friction impulse
        relativeVel = (bodyB->GetVelocity() + glm::cross(bodyB->GetAngularVelocity(), rB)) -
                     (bodyA->GetVelocity() + glm::cross(bodyA->GetAngularVelocity(), rA));

        glm::vec3 tangent = relativeVel - (contact.normal * glm::dot(relativeVel, contact.normal));
        float tangentLen = glm::length(tangent);

        if (tangentLen > 0.0001f) {
          tangent = tangent / tangentLen;

          float friction = std::sqrt(bodyA->GetFriction() * bodyB->GetFriction());

          crossA = glm::cross(rA, tangent);
          crossB = glm::cross(rB, tangent);

          float invMassSumTangent = bodyA->GetInverseMass() + bodyB->GetInverseMass() +
                                  glm::dot(crossA, bodyA->GetGlobalInverseInertiaTensor() * crossA) +
                                  glm::dot(crossB, bodyB->GetGlobalInverseInertiaTensor() * crossB);

          float tangentVel = glm::dot(relativeVel, tangent);
          float frictionImpulse = -tangentVel / invMassSumTangent;

          float maxFriction = friction * normalImpulse;
          frictionImpulse = glm::clamp(frictionImpulse, -maxFriction, maxFriction);

          glm::vec3 Pt = tangent * frictionImpulse;
          bodyA->ApplyImpulse(-Pt, rA);
          bodyB->ApplyImpulse(Pt, rB);

          std::cout << "Applying collision impulse. Bodies at:\n"
                      << "BodyA: y=" << contact.bodyA->GetPosition().y
                      << " vel=" << contact.bodyA->GetVelocity().y << "\n"
                      << "BodyB: y=" << contact.bodyB->GetPosition().y
                      << " vel=" << contact.bodyB->GetVelocity().y << "\n";
        }
      }
    }
  }
}