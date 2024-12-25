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
    for (const auto& body : m_rigidBodies) {
        // Skip static bodies as their AABBs don't change
        if (body->GetInverseMass() == 0.0f) {
            continue;
        }

        // Update AABBs for each collider
        for (auto& collider : body->GetColliders()) {
            AABB* aabb = collider.GetAABB();
            if (!aabb) {
                std::cout << "Warning: Null AABB found!\n";
                continue;
            }

            // Store old bounds for debug
            glm::vec3 oldMin = aabb->minPoint;
            glm::vec3 oldMax = aabb->maxPoint;

            // Get collider bounds in local space
            const BoxCollider* boxCollider = dynamic_cast<const BoxCollider*>(&collider);
            if (boxCollider) {
                glm::vec3 halfExtents = boxCollider->m_shape.m_halfExtents;
                glm::vec3 pos = body->GetPosition();

                // Transform to world space
                glm::mat3 orientation = body->GetOrientation();
                glm::vec3 worldHalfExtents;
                for (int i = 0; i < 3; i++) {
                    worldHalfExtents[i] = std::abs(orientation[0][i] * halfExtents.x) +
                                        std::abs(orientation[1][i] * halfExtents.y) +
                                        std::abs(orientation[2][i] * halfExtents.z);
                }

                // Set new AABB bounds
                aabb->minPoint = pos - worldHalfExtents;
                aabb->maxPoint = pos + worldHalfExtents;

                // Add small margin to prevent tunneling
                const float margin = 0.01f;
                aabb->minPoint -= glm::vec3(margin);
                aabb->maxPoint += glm::vec3(margin);

                std::cout << "Updated AABB for body at Y=" << pos.y
                         << " Old bounds: [" << oldMin.y << ", " << oldMax.y
                         << "] New bounds: [" << aabb->minPoint.y << ", " << aabb->maxPoint.y << "]\n";
            }
        }
    }
}


  void PhysicsWorld::StepSimulation(float dt) {

    for (const auto& body : m_rigidBodies) {
      if (body->GetInverseMass() > 0.0f) { // Only dynamic bodies
        glm::vec3 pos = body->GetPosition();
        glm::vec3 vel = body->GetVelocity();
      }
    }

    // Apply forces (gravity etc.)
    for (auto& rigidBody : m_rigidBodies) {
      if (rigidBody->GetInverseMass() > 0.0f) {
        glm::vec3 force = glm::vec3(0.0f, -9.81f * rigidBody->GetMass(), 0.0f);
        rigidBody->ApplyForce(force, rigidBody->GetPosition());
      }
    }

    // Integrate velocities and positions
    for (auto& rigidBody : m_rigidBodies) {
      if (rigidBody->GetInverseMass() > 0.0f) {
        glm::vec3 beforePos = rigidBody->GetPosition();
        glm::vec3 beforeVel = rigidBody->GetVelocity();

        rigidBody->Integrate(dt);

        glm::vec3 afterPos = rigidBody->GetPosition();
        glm::vec3 afterVel = rigidBody->GetVelocity();
      }
    }

    UpdateRigidBodyAABBs();
    m_broadphase->Update();

    // 4. collision detection
    const auto& potentialPairs = m_broadphase->ComputePairs();
    std::vector<ContactInfo> contacts;
    contacts.reserve(potentialPairs.size());

    for (const auto& pair : potentialPairs) {
      Collider* colliderA = pair.first;
      Collider* colliderB = pair.second;

      RigidBody* bodyA = colliderA->GetRigidBody();
      RigidBody* bodyB = colliderB->GetRigidBody();

      std::cout << "\nChecking collision between bodies at:\n"
                << "BodyA: y=" << bodyA->GetPosition().y
                << " (half-height=" << dynamic_cast<const BoxCollider*>(colliderA)->m_shape.m_halfExtents.y << ")\n"
                << "BodyB: y=" << bodyB->GetPosition().y
                << " (half-height=" << dynamic_cast<const BoxCollider*>(colliderB)->m_shape.m_halfExtents.y << ")\n";

      // Skip if both bodies are static
      if (bodyA->GetInverseMass() == 0.0f && bodyB->GetInverseMass() == 0.0f) {
        continue;
      }

      ContactInfo contact;
      if (GJKAlgorithm::DetectCollision(colliderA, colliderB, contact)) {
        std::cout << "Collision detected! Adding contact.\n";
        contact.bodyA = bodyA;
        contact.bodyB = bodyB;
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