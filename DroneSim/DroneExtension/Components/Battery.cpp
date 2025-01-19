#include "Battery.h"

namespace lark::battery {
  namespace {
    using battery_data = drone_components::component_data<drone_data::BatteryBody>;
    drone_components::component_pool<battery_id, battery_data> pool;
  }

  drone_component create(init_info info, drone_entity::entity entity) {
    return drone_component{ pool.create(info, entity) };
  }

  void remove(drone_component c) {
    pool.remove(c.get_id());
  }

  glm::mat4 get_transform(drone_component c) {
    return pool.get_transform(c.get_id());
  }

  void update_transform(drone_component c, glm::mat4& transform) {
    pool.set_transform(c.get_id(), transform);
  }
}