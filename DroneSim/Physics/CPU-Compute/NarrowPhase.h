#pragma once
#include "PhysicsData.h"
#include <glm/glm.hpp>
#include <vector>

namespace drosim::physics::cpu {

/**
 * @brief Main narrow phase collision detection function.
 * Takes potential collision pairs from broad phase and performs detailed collision detection.
 * For colliding pairs, generates contact points with normal and penetration depth.
 *
 * @param world The physics world containing all body and collider data
 * @param pairs Vector of potential collision pairs from broad phase (as tree node indices)
 */
void NarrowPhase(PhysicsWorld& world,
                 const std::vector<std::pair<uint32_t, uint32_t>>& pairs);

} // namespace drosim::physics::cpu