#include "PhysicsSystem.h"
#include "BodySystem.h"
#include "ColliderSystem.h"
#include "NarrowPhase.h"
#include "ConstraintSystem.h"

namespace drosim::physics::cpu {

  void StepSimulation(PhysicsWorld& world, float dt, int solverIterations) {
    // 1) Integrate forces
    IntegrateForces(world, dt);

    // 2) Update dynamic tree
    UpdateDynamicTree(world);

    // 3) Broad phase
    std::vector<std::pair<uint32_t, uint32_t>> potentialPairs;
    BroadPhaseCollisions(world, potentialPairs);

    // 4) Narrow phase
    NarrowPhase(world, potentialPairs);

    // 5) Solve constraints (contacts + user constraints)
    SolveConstraints(world, dt, solverIterations);

    // 6) Integrate velocities
    IntegrateVelocities(world, dt);

    // 7) Sleeping
    UpdateSleeping(world);

    // 8) Clear forces
    ClearForces(world);
  }
}
