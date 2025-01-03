#include "Fuselage.h"

namespace lark::fuselage {
  namespace {
    struct fuselage_data : public lark::drone_data::FuselageBody {
      bool is_valid{ false };
      drone_entity::drone_id drone_id{};
    };

    util::vector<fuselage_data> fuselages;
    util::vector<id::id_type> id_mapping;
    util::vector<id::generation_type> generations;
    std::deque<fuselage_id> free_ids;

    bool exists(fuselage_id id) {
      assert(id::is_valid(id));
      const id::id_type index{ id::index(id) };
      assert(index < generations.size() &&
             !(id::is_valid(id_mapping[index]) &&
               id_mapping[index] >= fuselages.size()));
      return (id::is_valid(id_mapping[index]) &&
              generations[index] == id::generation(id) &&
              fuselages[id_mapping[index]].is_valid);
    }
  }

  drone_component create(init_info info, drone_entity::entity entity) {
    assert(entity.is_valid());
    fuselage_id id{};

    if (free_ids.size() > id::min_deleted_elements) {
      id = free_ids.front();
      assert(!exists(id));
      free_ids.pop_front();
      id = fuselage_id{ id::new_generation(id) };
      ++generations[id::index(id)];
    } else {
      id = fuselage_id{ (id::id_type)id_mapping.size() };
      id_mapping.emplace_back();
      generations.push_back(0);
    }

    fuselage_data data;

    static_cast<lark::drone_data::FuselageBody&>(data) = info;

    assert(id::is_valid(id));
    const id::id_type index{ (id::id_type) fuselages.size()};
    data.is_valid = true;
    data.drone_id = entity.get_id();
    fuselages.emplace_back(std::move(data));

    id_mapping[id::index(id)] = index;
    return drone_component(id);
  };

  void remove(drone_component c) {
    if (!c.is_valid()) return;
    if (!exists(c.get_id())) return;

    const fuselage_id id{ c.get_id() };
    const id::id_type index{ id_mapping[id::index(id)] };
    const id::id_type last_index{ (id::id_type)fuselages.size() - 1 };

    // Move last element to the removed position
    if (index != last_index) {
      fuselages[index] = std::move(fuselages[last_index]);
      // Update the id_mapping for the moved element
      const auto moved_id = std::find_if(id_mapping.begin(), id_mapping.end(),
          [last_index](id::id_type mapping) { return mapping == last_index; });
      if (moved_id != id_mapping.end()) {
        *moved_id = index;
      }
    }

    fuselages.pop_back();
    id_mapping[id::index(id)] = id::invalid_id;

    if (generations[id::index(id)] < id::max_generation) {
      free_ids.push_back(id);
    }
  };
}