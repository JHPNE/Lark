#include "Geometry.h"
#include "../Common/CommonHeaders.h"

namespace lark::geometry {
    namespace {
        // Replace individual arrays with a single vector of geometry data
        struct geometry_data {
            bool is_valid{ false };
            bool is_dynamic{ false };
            std::shared_ptr<tools::scene> scene{ nullptr };
        };

        util::vector<geometry_data> geometries;
        util::vector<id::id_type> id_mapping;
        util::vector<id::generation_type> generations;
        std::deque<geometry_id> free_ids;

        bool exists(const geometry_id id) {
            assert(id::is_valid(id));
            const id::id_type index{ id::index(id) };
            assert(index < generations.size() && 
                   !(id::is_valid(id_mapping[index]) && 
                     id_mapping[index] >= geometries.size()));
            return (id::is_valid(id_mapping[index]) &&
                    generations[index] == id::generation(id) &&
                    geometries[id_mapping[index]].is_valid);
        }
    }

    component create(init_info info, game_entity::entity entity) {
        assert(entity.is_valid());
        assert(info.scene);
        
        geometry_id id{};
        
        if (free_ids.size() > id::min_deleted_elements) {
            id = free_ids.front();
            assert(!exists(id));
            free_ids.pop_front();
            id = geometry_id{ id::new_generation(id) };
            ++generations[id::index(id)];
        } else {
            id = geometry_id{ (id::id_type)id_mapping.size() };
            id_mapping.emplace_back();
            generations.push_back(0);
        }

        assert(id::is_valid(id));
        const id::id_type index{ (id::id_type)geometries.size() };
        geometries.emplace_back(geometry_data{
            true,
            info.is_dynamic,
            info.scene,
        });
        
        id_mapping[id::index(id)] = index;
        return component{ id };
    }

    void remove(component c) {
        if (!c.is_valid()) return;
        if (!exists(c.get_id())) return;

        const geometry_id id{ c.get_id() };
        const id::id_type index{ id_mapping[id::index(id)] };
        const id::id_type last_index{ (id::id_type)geometries.size() - 1 };

        // Move last element to the removed position
        if (index != last_index) {
            geometries[index] = std::move(geometries[last_index]);
            // Update the id_mapping for the moved element
            const auto moved_id = std::find_if(id_mapping.begin(), id_mapping.end(),
                [last_index](id::id_type mapping) { return mapping == last_index; });
            if (moved_id != id_mapping.end()) {
                *moved_id = index;
            }
        }
        
        geometries.pop_back();
        id_mapping[id::index(id)] = id::invalid_id;

        if (generations[id::index(id)] < id::max_generation) {
            free_ids.push_back(id);
        }
    }

    // Update the component methods to use the new data structure
    std::shared_ptr<tools::scene> component::get_scene() const {
        assert(is_valid() && exists(_id));
        return geometries[id_mapping[id::index(_id)]].scene;
    }

    bool component::set_dynamic(bool dynamic) {
        assert(is_valid() && exists(_id));
        geometries[id_mapping[id::index(_id)]].is_dynamic = dynamic;
        return true;
    }

    bool component::is_dynamic() const {
        assert(is_valid() && exists(_id));
        return geometries[id_mapping[id::index(_id)]].is_dynamic;
    }

    bool component::update_vertices(const std::vector<math::v3>& new_positions) {
        assert(is_valid() && exists(_id));
        const auto& geom = geometries[id_mapping[id::index(_id)]];
        assert(geom.is_dynamic && "Geometry must be dynamic to update vertices");

        geometry_import_settings settings{};
        settings.calculate_normals = true;
        settings.calculate_tangents = true;
        settings.smoothing_angle = 178.f;

        return tools::update_scene_mesh_positions(*geom.scene, 0, 0, new_positions, settings);
    }

    void shutdown() {
        geometries.clear();
        id_mapping.clear();
        generations.clear();
        free_ids.clear();
    }
}
