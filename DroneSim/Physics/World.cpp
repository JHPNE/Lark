#include "World.h"

namespace drosim::physics {
  void World::stepSimulation(float deltaTime) {
    m_dynamicsWorld->stepSimulation(deltaTime);
  }
}