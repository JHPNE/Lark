// Component.h
#pragma once
#include "../DroneCommonHeaders.h"

namespace lark::drone_components {
  template<typename BaseData>
  struct component_data : public BaseData {
    using base_type = BaseData;
    bool is_valid{ false };
    drone_entity::drone_id drone_id{};

    component_data() = default;
    explicit component_data(const BaseData& base) : BaseData(base) {}
  };

  template<typename ComponentId, typename Data>
  class component_pool {
  private:
    util::vector<Data> elements;
    util::vector<id::id_type> id_mapping;
    util::vector<id::generation_type> generations;
    std::deque<ComponentId> free_ids;

  public:
    [[nodiscard]] bool exists(ComponentId id) const {
      assert(id::is_valid(id));
      const id::id_type index{ id::index(id) };
      assert(index < generations.size() &&
             !(id::is_valid(id_mapping[index]) &&
               id_mapping[index] >= elements.size()));
      return (id::is_valid(id_mapping[index]) &&
              generations[index] == id::generation(id) &&
              elements[id_mapping[index]].is_valid);
    }

    template<typename InitInfo>
    ComponentId create(const InitInfo& info, drone_entity::entity entity) {
      static_assert(std::is_base_of_v<typename Data::base_type, InitInfo>,
                   "InitInfo must inherit from the component's base type");

      assert(entity.is_valid());
      ComponentId id{};

      if (free_ids.size() > id::min_deleted_elements) {
        id = free_ids.front();
        assert(!exists(id));
        free_ids.pop_front();
        id = ComponentId{ id::new_generation(id) };
        ++generations[id::index(id)];
      } else {
        id = ComponentId{ (id::id_type)id_mapping.size() };
        id_mapping.emplace_back();
        generations.push_back(0);
      }

      Data data;
      // Copy base class members
      static_cast<typename Data::base_type&>(data) = static_cast<const typename Data::base_type&>(info);

      assert(id::is_valid(id));
      const id::id_type index{ (id::id_type)elements.size() };
      data.is_valid = true;
      data.drone_id = entity.get_id();
      elements.emplace_back(std::move(data));

      id_mapping[id::index(id)] = index;
      return id;
    }

    void remove(ComponentId id) {
      if (!id::is_valid(id)) return;
      if (!exists(id)) return;

      const id::id_type index{ id_mapping[id::index(id)] };
      const id::id_type last_index{ (id::id_type)elements.size() - 1 };

      // Move last element to the removed position
      if (index != last_index) {
        elements[index] = std::move(elements[last_index]);
        // Update the id_mapping for the moved element
        const auto moved_id = std::find_if(id_mapping.begin(), id_mapping.end(),
          [last_index](id::id_type mapping) { return mapping == last_index; });
        if (moved_id != id_mapping.end()) {
          *moved_id = index;
        }
      }

      elements.pop_back();
      id_mapping[id::index(id)] = id::invalid_id;

      if (generations[id::index(id)] < id::max_generation) {
        free_ids.push_back(id);
      }
    }

    void set_transform(ComponentId id, const glm::mat4& new_transform) {
      if (!exists(id)) return;
      elements[id_mapping[id::index(id)]].transform = new_transform;

      // Update physics body if it exists
      auto* body = &elements[id_mapping[id::index(id)]];
      if (body->rigidBody) {
        btTransform bt_transform;
        btMatrix3x3 basis;
        btVector3 origin;

        basis.setValue(
            float(new_transform[0][0]), float(new_transform[0][1]), float(new_transform[0][2]),
            float(new_transform[1][0]), float(new_transform[1][1]), float(new_transform[1][2]),
            float(new_transform[2][0]), float(new_transform[2][1]), float(new_transform[2][2])
        );

        origin.setValue(
            float(new_transform[3][0]),
            float(new_transform[3][1]),
            float(new_transform[3][2])
        );

        bt_transform.setBasis(basis);
        bt_transform.setOrigin(origin);

        body->rigidBody->setWorldTransform(bt_transform);
        body->rigidBody->activate(true);
      }
    }

    [[nodiscard]] glm::mat4 get_transform(ComponentId id) const {
      return exists(id) ? elements[id_mapping[id::index(id)]].transform : glm::mat4(1.0f);
    }

    // Access to component data
    [[nodiscard]] Data* get_data(ComponentId id) {
      return exists(id) ? &elements[id_mapping[id::index(id)]] : nullptr;
    }
    [[nodiscard]] const Data* get_data(ComponentId id) const {
      return exists(id) ? &elements[id_mapping[id::index(id)]] : nullptr;
    }

    [[nodiscard]] const util::vector<Data>& get_all_components() const {
      return elements;
    }
  };
}