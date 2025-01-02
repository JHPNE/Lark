#include "DroneCreation.h"

namespace drosim::drone {
  namespace {

    util::vector<RotorBody> rotors;
    util::vector<id::generation_type> generations;
    util::deque<drone_id> free_ids;

    util::vector<drone_id> active_entities;
  }

  bool is_alive(drone_id id) {
    assert(id::is_valid(id));
    const id::id_type index{ id::index(id) };
    assert(index < generations.size());
    return (generations[index] == id::generation(id));
  }

  void create_rotor(drone_id id, RotorBody& body) {
    assert(id::is_valid(id));

  }

  drone_id createDrone(DroneData &droneData) {

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
    };


    for (const auto& body : droneData.bodies) {
      switch (body->type) {
        case BodyType::ROTOR:
          if (auto* rotor = dynamic_cast<RotorBody*>(body.get())) {
            create_rotor(id, *rotor);
          }
          break;
        default:
          break;
      }
    }

    const id::id_type index{ id::index(id) };
    return id;
  }


}