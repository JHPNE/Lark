#include "Rotor.h"

namespace lark::rotor {
    namespace {
        using rotor_data = drone_components::component_data<drone_data::RotorBody>;
        drone_components::component_pool<rotor_id, rotor_data> pool;
    }

    drone_component create(init_info info, drone_entity::entity entity) {
        return drone_component{ pool.create(info, entity)};
    }

    void remove(drone_component c) {
        pool.remove(c.get_id());
    }
};