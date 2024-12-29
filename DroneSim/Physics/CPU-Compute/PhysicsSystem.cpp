#include "PhysicsSystem.h"
#include "BodySystem.h"
#include "ColliderSystem.h"
#include "ConstraintSystem.h"
#include "NarrowPhase.h"

#include <iostream>

namespace drosim::physics::cpu {

  void StepSimulation(PhysicsWorld& world, float dt, int solverIterations) {
    /*

    // Print physics state every second
    if (accumulatedTime >= 1.0f) {
      std::cout << "Physics State Check:\n";
      std::cout << "Active bodies: " << world.bodyPool.Size() << "\n";
      std::cout << "Current dt: " << dt << "\n";
      accumulatedTime = 0.0f;
    }
    */

    // 1) Integrate forces
    IntegrateForces(world, dt);
    /*
    for (size_t i = 0; i < world.bodyPool.Size(); ++i) {
      auto& body = world.bodyPool[i];
      if (glm::any(glm::isnan(body.motion.velocity))) {
        std::cout << "NaN velocity detected after force integration for body " << i << "\n";
      }
    }
    */

    // 2) Update dynamic tree
    UpdateDynamicTree(world);

    // 3) Broad phase
    std::vector<std::pair<uint32_t, uint32_t>> potentialPairs;
    BroadPhaseCollisions(world, potentialPairs);

    // 4) Narrow phase
    size_t prevContacts = world.contacts.size();
    NarrowPhase(world, potentialPairs);

    /*
    for (size_t i = 0; i < std::min(world.contacts.size(), size_t(3)); ++i) {
      const auto& contact = world.contacts[i];
      std::cout << "Contact " << i << ":"
                << " Bodies: " << contact.bodyAIndex << "-" << contact.bodyBIndex
                << " Penetration: " << contact.penetration << "\n";
    }
    */

    // 5) Solve constraints (contacts + user constraints)
    SolveConstraints(world, dt, solverIterations);

    // 6) Integrate velocities
    IntegrateVelocities(world, dt);
    /*
    for (size_t i = 0; i < world.bodyPool.Size(); ++i) {
      auto& body = world.bodyPool[i];
      if (glm::any(glm::isnan(body.motion.position))) {
        std::cout << "NaN position detected after velocity integration for body " << i << "\n";
      }
    }
    */

    // 7) Sleeping
    //UpdateSleeping(world);
    /*
    size_t activeCount = 0;
    for (size_t i = 0; i < world.bodyPool.Size(); ++i) {
      if (world.bodyPool[i].flags.active) activeCount++;
    }
    std::cout << "Active bodies: " << activeCount << "/" << world.bodyPool.Size() << "\n";
    */


    // 8) Clear forces
    ClearForces(world);
  }
}
