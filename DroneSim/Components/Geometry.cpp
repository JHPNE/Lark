#include "Geometry.h"
#include "../Common/CommonHeaders.h"

namespace drosim::geometry {

    namespace {
        struct geometry_component {
            tools::scene* scene;
            tools::mesh* mesh;
            tools::lod_group* lod_group;
            bool is_valid;
            bool is_dynamic;
        };

        util::vector<geometry_component> geometry;
    }

    component create(init_info info, game_entity::entity entity) {
        assert(info.scene || info.mesh || info.lod_group); // At least one geometry source must be provided
        assert(entity.is_valid());
        const id::id_type entity_index{ id::index(entity.get_id()) };

        geometry.emplace_back(geometry_component{
            info.scene,
            info.mesh,
            info.lod_group,
            true,
            info.is_dynamic
        });

        // If dynamic is requested, set the mesh as dynamic
        auto& comp = geometry.back();
        if (comp.is_dynamic && comp.mesh) {
            comp.mesh->set_dynamic(true);
        }

        return component(geometry_id{ (id::id_type)geometry.size() - 1 });
    }

    void remove(geometry_id id) {
        assert(id::is_valid(id));
        const id::id_type index{ id::index(id) };
        assert(index < geometry.size());
        assert(geometry[index].is_valid);
        
        geometry[index].is_valid = false;
        geometry[index].scene = nullptr;
        geometry[index].mesh = nullptr;
        geometry[index].lod_group = nullptr;
        geometry[index].is_dynamic = false;
    }

    tools::scene* component::get_scene() const {
        assert(is_valid());
        return geometry[_id].scene;
    }

    tools::mesh* component::get_mesh() const {
        assert(is_valid());
        return geometry[_id].mesh;
    }

    tools::lod_group* component::get_lod_group() const {
        assert(is_valid());
        return geometry[_id].lod_group;
    }

    bool component::set_dynamic(bool dynamic) {
        assert(is_valid());
        auto& comp = geometry[_id];
        if (!comp.mesh) return false;

        comp.is_dynamic = dynamic;
        comp.mesh->set_dynamic(dynamic);
        return true;
    }

    bool component::update_vertices(const std::vector<math::v3>& new_positions) {
        assert(is_valid());
        auto& comp = geometry[_id];
        if (!comp.mesh || !comp.is_dynamic) return false;

        try {
            comp.mesh->update_vertices(new_positions);
            return true;
        }
        catch (const std::runtime_error&) {
            return false;
        }
    }

    bool component::recalculate_normals() {
        assert(is_valid());
        auto& comp = geometry[_id];
        if (!comp.mesh || !comp.is_dynamic) return false;

        try {
            comp.mesh->recalculate_normals();
            return true;
        }
        catch (const std::runtime_error&) {
            return false;
        }
    }

    bool component::is_dynamic() const {
        assert(is_valid());
        return geometry[_id].is_dynamic;
    }
}
