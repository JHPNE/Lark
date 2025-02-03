#include "Multirotor.h"

namespace lark::drones {
  glm::mat3 Multirotor::hatMap(const glm::vec3& v) {
    return glm::mat3(
          0.0f, -v.z, v.y,
          v.z, 0.0f, -v.x,
          -v.y, v.x, 0.0f
    );
  }
}