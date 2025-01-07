#include "Fuselage.h"

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
}