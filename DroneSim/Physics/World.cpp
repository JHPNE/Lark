#include "World.h"

namespace lark::physics {
  void World::stepSimulation(float deltaTime) {
    m_dynamicsWorld->stepSimulation(deltaTime);
  }
}