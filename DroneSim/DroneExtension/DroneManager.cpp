#include "DroneManager.h"
#include "Components/Fuselage.h"
#include "Components/Battery.h"
#include "Components/Rotor.h"

namespace lark::drone_entity {
  namespace {
    //TODO maybe only have one and some other bodies instead
    util::vector<fuselage::drone_component> fuselages;
    util::vector<battery::drone_component> batteries;
    util::vector<rotor::drone_component> rotors;

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

      fuselages.emplace_back();
      batteries.emplace_back();
      rotors.emplace_back();
    }

    const entity new_entity{ id };
    const id::id_type index{ id::index(id) };

    assert(!fuselages[index].is_valid());
    fuselages[index] = fuselage::create(*info.fuselage, new_entity);
    if (!fuselages[index].is_valid()) return {};

    // this will repeat for x components which i dont like
    if (info.battery) {
      assert(!batteries[index].is_valid());
      batteries[index] = battery::create(*info.battery, new_entity);
      //batteries::batteryCalculateCharge(batteries[index]);
      assert(batteries[index].is_valid());
    }

    if (info.rotor) {
      assert(!rotors[index].is_valid());
      rotors[index] = rotor::create(*info.rotor, new_entity);
      assert(rotors[index].is_valid());
    }

    return new_entity;
  };

  void remove(drone_id id) {
    const id::id_type index{ id::index(id) };
    assert(is_alive(id));

    fuselage::remove(fuselages[index]);
    fuselages[index] = {};

    battery::remove(batteries[index]);
    batteries[index] = {};

    if (generations[index] < id::max_generation) {
      free_ids.push_back(id);
    }
  }

  bool is_alive(drone_id id) {
    assert(id::is_valid(id));
    const id::id_type index{ id::index(id) };
    assert(index < generations.size());
    return (generations[index] == id::generation(id) && fuselages[index].is_valid());
  }

  void addDroneComponent(const drone_id id, const drone_data::BodyType component,
                         const entity_info info) {
    const id::id_type index{ id::index(id) };
    assert(is_alive(id));

    switch (component) {
    case drone_data::BodyType::FUSELAGE:
      // We only want one fuselage
      if (!fuselages[index].is_valid()) {
        fuselages[index] = fuselage::create(*info.fuselage, entity{ id });
      }
      break;
    case drone_data::BodyType::BATTERY:
      if (!batteries[index].is_valid()) {
        batteries[index] = battery::create(*info.battery, entity{ id });
      }
      break;
    case drone_data::BodyType::ROTOR:
      break;
    default:
      break;
    }
  }

  rotor::drone_component entity::rotor() const {
    assert(is_alive(_id));
    const id::id_type index{ id::index(_id) };
    return rotors[index];
  }

/*
transform::component entity::transform() const {
    assert(is_alive(_id));
    const id::id_type index{ id::index(_id) };
    return transforms[index];
  }

script::component entity::script() const {
    assert(is_alive(_id));
    return scripts[id::index(_id)];
  }

geometry::component entity::geometry() const {
    assert(is_alive(_id));
    return geometries[id::index(_id)];
  }
  */
};