#include "PhysicsWorld.h"

#include <iostream>

namespace drosim::physics {

  std::shared_ptr<RigidBody> PhysicsWorld::CreateRigidBody() {
    auto body = std::make_shared<RigidBody>();
    m_rigidBodies.push_back(body);
    return body;
  }

 void PhysicsWorld::UpdateRigidBodyAABBs() {
    // For each rigid body
    for (const auto& body : m_rigidBodies) {
        // Skip static bodies as their AABBs don't change
        if (body->GetInverseMass() == 0.0f) {
            continue;
        }

        // Update AABBs for each collider
        for (auto& collider : body->GetColliders()) {
            AABB* aabb = collider.GetAABB();
            if (!aabb) continue;

            // Reset AABB to empty
            aabb->minPoint = glm::vec3(std::numeric_limits<float>::max());
            aabb->maxPoint = glm::vec3(-std::numeric_limits<float>::max());

            // Sample support points along principal axes
            const glm::vec3 axes[6] = {
                glm::vec3(1, 0, 0), glm::vec3(-1, 0, 0),
                glm::vec3(0, 1, 0), glm::vec3(0, -1, 0),
                glm::vec3(0, 0, 1), glm::vec3(0, 0, -1)
            };

            // Find support points in all 6 principal directions
            for (const auto& dir : axes) {
                // Convert direction to local space
                glm::vec3 localDir = body->GlobalToLocalVec(dir);

                // Get support point in local space
                glm::vec3 support = collider.Support(localDir);

                // Transform to world space
                glm::vec3 worldSupport = body->LocalToGlobal(support);

                // Expand AABB
                aabb->Expand(worldSupport);
            }

            // Add a small margin to prevent tunneling
            const float margin = 0.01f;
            glm::vec3 marginVec(margin);
            aabb->minPoint -= marginVec;
            aabb->maxPoint += marginVec;

            // Ensure AABB is valid after computation
            if (!aabb->IsValid()) {
                // Reset to a small valid AABB around the body's position
                glm::vec3 pos = body->GetPosition();
                aabb->minPoint = pos - glm::vec3(0.1f);
                aabb->maxPoint = pos + glm::vec3(0.1f);
            }
        }
    }
}


  void PhysicsWorld::StepSimulation(float dt) {

    std::cout << "Step begin - Bodies: " << m_rigidBodies.size() << "\n";
    for (const auto& body : m_rigidBodies) {
      if (body->GetInverseMass() > 0.0f) { // Only dynamic bodies
        glm::vec3 pos = body->GetPosition();
        glm::vec3 vel = body->GetVelocity();
        std::cout << "Body state - Pos: " << pos.y << " Vel: " << vel.y << "\n";
      }
    }

    // Apply forces (gravity etc.)
    for (auto& rigidBody : m_rigidBodies) {
      if (rigidBody->GetInverseMass() > 0.0f) {
        glm::vec3 force = glm::vec3(0.0f, -9.81f * rigidBody->GetMass(), 0.0f);
        std::cout << "Applying gravity force: " << force.y << "\n";
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

        std::cout << "Integration - Before pos: " << beforePos.y
                 << " After pos: " << afterPos.y
                 << " Before vel: " << beforeVel.y
                 << " After vel: " << afterVel.y << "\n";
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

      // Skip if both bodies are static
      if (bodyA->GetInverseMass() == 0.0f && bodyB->GetInverseMass() == 0.0f) {
        continue;
      }

      ContactInfo contact;
      if (GJKAlgorithm::DetectCollision(colliderA, colliderB, contact)) {
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
        }
      }
    }
  }
}