#include "Geometry.h"
#include "../Common/CommonHeaders.h"

namespace drosim::geometry {

    namespace {
        // Arrays storing component data
        util::vector<bool>                geometry_valid;
        util::vector<tools::scene*>       geometry_scenes;
        util::vector<bool>                geometry_is_dynamic;
        util::vector<game_entity::entity_id> geometry_entities;

        util::vector<id::generation_type> geometry_generations;
        std::deque<geometry_id>           free_ids;

        inline bool exists(geometry_id id) {
            if (!id::is_valid(id)) return false;
            const auto index = id::index(id);
            if (index >= geometry_valid.size()) return false;
            if (!geometry_valid[index]) return false;
            return (geometry_generations[index] == id::generation(id));
        }
    }

    component create(init_info info, game_entity::entity entity) {
        assert(entity.is_valid() && "Entity must be valid");
        assert(info.scene && "A valid scene pointer must be provided");

        geometry_id id{};
        // Reuse a free id if available
        if (!free_ids.empty()) {
            id = free_ids.front();
            free_ids.pop_front();
            id = geometry_id{ id::new_generation(id) };
            ++geometry_generations[id::index(id)];
        } else {
            // Allocate a new slot
            const auto index = static_cast<id::id_type>(geometry_valid.size());

            geometry_valid.push_back(true);
            geometry_scenes.push_back(info.scene);
            geometry_is_dynamic.push_back(info.is_dynamic);
            geometry_entities.push_back(entity.get_id());
            geometry_generations.push_back(0);

            id = geometry_id{ index };
        }

        // Validate
        assert(id::is_valid(id));
        return component{ id };
    }

    void remove(component c) {
        if (!c.is_valid()) return;
        if (!exists(c.get_id())) return;

        const geometry_id id = c.get_id();
        const auto index = id::index(id);
        geometry_valid[index] = false;
        geometry_scenes[index] = nullptr;
        geometry_is_dynamic[index] = false;
        geometry_entities[index] = game_entity::entity_id{id::invalid_id};

        // Make this id available for reuse if generations allow
        if (geometry_generations[index] < id::max_generation) {
            free_ids.push_back(id);
        }
    }

    void shutdown() {
        // Clear all geometry data
        geometry_valid.clear();
        geometry_scenes.clear();
        geometry_is_dynamic.clear();
        geometry_entities.clear();
        geometry_generations.clear();
        free_ids.clear();
    }

    tools::scene* component::get_scene() const {
        assert(is_valid());
        const auto index = id::index(_id);
        return geometry_scenes[index];
    }

    bool component::set_dynamic(bool dynamic) {
        assert(is_valid());
        const auto index = id::index(_id);
        geometry_is_dynamic[index] = dynamic;
        return true;
    }

    bool component::is_dynamic() const {
        assert(is_valid());
        const auto index = id::index(_id);
        return geometry_is_dynamic[index];
    }

    bool component::update_vertices(const std::vector<math::v3> &new_positions) {
        assert(is_valid());
        const auto index = id::index(_id);
        assert(exists(index));

        auto lod_groups = geometry_scenes[index]->lod_groups;
        for (auto lod_group : lod_groups) {
            for (auto mesh : lod_group.meshes) {
                mesh.update_vertices(new_positions);
            }
        }
        return false;
    }


}
