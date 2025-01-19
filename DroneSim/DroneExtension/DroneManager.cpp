#include "DroneManager.h"
#include "Components/Fuselage.h"
#include "Components/Battery.h"
#include "Components/Rotor.h"

namespace lark::drone_entity {
  namespace {
    util::vector<fuselage::drone_component> fuselages;
    util::vector<util::vector<battery::drone_component>> batteries;
    util::vector<util::vector<rotor::drone_component>> rotors;

    std::vector<id::generation_type> generations;
    util::deque<drone_id> free_ids;

    auto create_battery_component(const battery::init_info& info, const entity& e) {
      return battery::create(info, e);
    }

    auto create_rotor_component(const rotor::init_info& info, const entity& e) {
      return rotor::create(info, e);
    }

    template<typename ComponentType, typename InitInfo>
    void create_components(const entity& new_entity,
                         const util::vector<InitInfo*>& info_array,
                         util::vector<ComponentType>& entity_components) {
      for (auto* component_info : info_array) {
        if (component_info) {
          // BATTERY
          if constexpr (std::is_same_v<ComponentType, battery::drone_component>) {
            auto component = create_battery_component(*component_info, new_entity);
            if (component.is_valid()) {
              entity_components.push_back(component);
            }
          }
          // ROTOR
          else if constexpr (std::is_same_v<ComponentType, rotor::drone_component>) {
            auto component = create_rotor_component(*component_info, new_entity);
            if (component.is_valid()) {
              entity_components.push_back(component);
            }
          }
        }
      }
    }

    template<typename ComponentType>
    void remove_components(util::vector<ComponentType>& components) {
      for (auto& component : components) {
        if constexpr (std::is_same_v<ComponentType, battery::drone_component>) {
          battery::remove(component);
        }
        else if constexpr (std::is_same_v<ComponentType, rotor::drone_component>) {
          rotor::remove(component);
        }
      }
      components.clear();
    }
  }


  entity create(entity_info info) {
    assert(info.fuselage);

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

    create_components<battery::drone_component>(new_entity, info.batteries, batteries[index]);
    create_components<rotor::drone_component>(new_entity, info.rotors, rotors[index]);
    return new_entity;
  };

  void remove(drone_id id) {
    const id::id_type index{ id::index(id) };
    assert(is_alive(id));

    fuselage::remove(fuselages[index]);
    fuselages[index] = {};

    remove_components(batteries[index]);
    remove_components(rotors[index]);

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

  bool add_component(const drone_id id, const drone_data::BodyType component_type,
                  const entity_info& info) {
    const id::id_type index{ id::index(id) };
    assert(is_alive(id));

    const entity entity_ref{ id };
    bool success = false;

    switch (component_type) {
    case drone_data::BodyType::FUSELAGE:
      if (!fuselages[index].is_valid()) {
        fuselages[index] = fuselage::create(*info.fuselage, entity{ id });
      }
      break;

    case drone_data::BodyType::BATTERY:
      if (!info.batteries.empty()) {
        create_components<battery::drone_component>(entity_ref, info.batteries, batteries[index]);
        success = true;
      }
      break;

    case drone_data::BodyType::ROTOR:
      if (!info.rotors.empty()) {
        create_components<rotor::drone_component>(entity_ref, info.rotors, rotors[index]);
        success = true;
      }
      break;
    }

    return success;
  }

  void transform(drone_id id, glm::mat4& new_transform) {
    assert(is_alive(id));
    const id::id_type index{ id::index(id) };

    // Fuselage as pivot reference
    const auto& fuselage = fuselages[index];
    if (!fuselage.is_valid()) return;

    // Calculate the transformation delta from original to new transform
    // This represents how much we need to move/rotate from current to desired state
    glm::mat4 original_transform = get_transform(fuselage);
    glm::mat4 delta_transform = new_transform * glm::inverse(original_transform);

    // First update the fuselage
    update_transform(fuselage, new_transform);

    // Update all comps
    for (auto& rotor : rotors[index]) {
      if (rotor.is_valid()) {
        glm::mat4 rotor_transform = get_transform(rotor);

        glm::mat4 new_rotor_transform = delta_transform * rotor_transform;
        update_transform(rotor, new_rotor_transform);
      }
    }

    for (auto& battery : batteries[index]) {
      if (battery.is_valid()) {
        glm::mat4 battery_transform = get_transform(battery);
        // Apply the same transformation relative to the pivot
        glm::mat4 new_battery_transform = delta_transform * battery_transform;
        update_transform(battery, new_battery_transform);
      }
    }
  }

  fuselage::drone_component entity::fuselage() const {
    assert(is_alive(_id));
    const id::id_type index{ id::index(_id) };
    return fuselages[index];
  }

  util::vector<battery::drone_component> entity::battery() const {
    assert(is_alive(_id));
    const id::id_type index{ id::index(_id) };
    return batteries[index];
  }

  util::vector<rotor::drone_component> entity::rotor() const {
    assert(is_alive(_id));
    const id::id_type index{ id::index(_id) };
    return rotors[index];
  }
};