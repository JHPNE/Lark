#include "Fuselage.h"

#include "BaseComponent.h"

#include <glm/gtc/type_ptr.hpp>

namespace lark::fuselage {
  namespace {
    using fuselage_data = drone_components::component_data<drone_data::FuselageBody>;
    drone_components::component_pool<fuselage_id, fuselage_data> pool;
  }

  drone_component create(init_info info, drone_entity::entity entity) {
    return drone_component{ pool.create(info, entity) };
  }

  void remove(drone_component c) {
    pool.remove(c.get_id());
  }

  glm::mat4 get_transform(drone_component t) {
    return pool.get_transform(t.get_id());
  }

  void update_transform(drone_component c, glm::mat4& new_transform) {
    pool.set_transform(c.get_id(), new_transform);
  }
}