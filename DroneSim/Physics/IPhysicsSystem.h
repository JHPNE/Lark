#pragma once
#include "CommonHeaders.h"

namespace drosim::physics {
class IPhysicsSystem {
public:
  virtual void Initialize() = 0;
  virtual void Update(float deltaTime) = 0;
  virtual ~IPhysicsSystem() = default;
};
}
