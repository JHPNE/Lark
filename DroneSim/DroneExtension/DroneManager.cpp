#include "DroneManager.h"
#include "Components/Fuselage.h"
#include "Components/Battery.h"

namespace lark::drone_entity {
  namespace {
    //TODO maybe only have one and some other bodies instead
    util::vector<fuselage::drone_component> fuselage;
    util::vector<battery::drone_component> battery;

    std::vector<id::generation_type> generations;
    util::deque<drone_id> free_ids;

  }


  entity create(entity_info info) {
    assert(info.fuselage);
    if (!info.fuselage) return {};

    drone_id id;
    if (free_ids.size() > id::min_deleted_elements) {
      id = free_ids.front();
      assert(!is_alive(id));
      free_ids.pop_front();
      id = drone_id{ id::new_generation(id) };
      ++generations[id::index(id)];
    } else {
      id = drone_id{(id::id_type) generations.size()};
      generations.push_back(0);

      fuselage.emplace_back();
      battery.emplace_back();
    }

    const entity new_entity{ id };
    const id::id_type index{ id::index(id) };

    assert(!fuselage[index].is_valid());
    fuselage[index] = fuselage::create(*info.fuselage, new_entity);
    if (!fuselage[index].is_valid()) return {};

    if (info.battery) {
      assert(!battery[index].is_valid());
      battery[index] = battery::create(*info.battery, new_entity);
      //battery::batteryCalculateCharge(battery[index]);
      assert(battery[index].is_valid());
    }

    return new_entity;
  };

  void remove(drone_id id) {
    const id::id_type index{ id::index(id) };
    assert(is_alive(id));

    fuselage::remove(fuselage[index]);
    fuselage[index] = {};

    if (generations[index] < id::max_generation) {
      free_ids.push_back(id);
    }
  }

  bool is_alive(drone_id id) {
    assert(id::is_valid(id));
    const id::id_type index{ id::index(id) };
    assert(index < generations.size());
    return (generations[index] == id::generation(id) && fuselage[index].is_valid());
  }

  void addDroneComponent(const drone_id id, const drone_components component,
                         const entity_info info) {
    const id::id_type index{ id::index(id) };
    assert(is_alive(id));

    switch (component) {
    case drone_components::FUSELAGE:
      // We only want one fuselage
      if (!fuselage[index].is_valid()) {
        fuselage[index] = fuselage::create(*info.fuselage, entity{ id });
      }
      break;
    case drone_components::ROTOR:
      break;
    default:
      break;
    }
  }
};