/*
#include "Common.h"
#include "CommonHeaders.h"
#include "Id.h"
#include "../DroneSim/Components/Entity.h"
#include "../DroneSim/Components/Transform.h"
#include "../DroneSim/Components/Script.h"
#include <glm/gtx/euler_angles.hpp>

using namespace drosim;

namespace {
    struct transform_component {
        f32 position[3];
        f32 rotation[3];
        f32 scale[3];
        transform::init_info to_init_info() {
            transform::init_info info{};
            // Copy position and scale directly
            memcpy(&info.position[0], &position[0], sizeof(f32) * 3);
            memcpy(&info.scale[0], &scale[0], sizeof(f32) * 3);

            // Convert Euler angles to quaternion using GLM
            glm::vec3 euler_angles(rotation[0], rotation[1], rotation[2]);
            glm::quat quaternion = glm::quat(euler_angles);

            // Store the quaternion components
            info.rotation[0] = quaternion.x;
            info.rotation[1] = quaternion.y;
            info.rotation[2] = quaternion.z;
            info.rotation[3] = quaternion.w;

            return info;
        }
    };

    struct script_component {
        script::detail::script_creator script_creator;
        script::init_info to_init_info() {
            script::init_info info{};
            info.script_creator = script_creator;
            return info;
        }
    };

    struct game_entity_descriptor {
        transform_component transform;
        script_component script;
    };

    util::vector<bool> active_entities;

    game_entity::entity entity_from_id(id::id_type id) {
        return game_entity::entity{ game_entity::entity_id(id) };
    }

    bool is_entity_valid(id::id_type id) {
        if (!id::is_valid(id)) return false;
        const id::id_type index{ id::index(id) };
        if (index >= active_entities.size()) return false;
        return active_entities[index];
    }

    void remove_entity(id::id_type id) {
        if (!is_entity_valid(id)) return;

        game_entity::entity_id entity_id{ id };
        if (game_entity::is_alive(entity_id)) {
            auto entity = game_entity::entity{ entity_id };
            // Handle script first
            if (auto script_comp = entity.script(); script_comp.is_valid()) {
                script::remove(script_comp);
            }
            // Then remove the entity
            game_entity::remove(entity_id);
        }

        // Mark as inactive after all removals
        const auto index = id::index(id);
        active_entities[index] = false;
    }
}

namespace engine {
    void cleanup_engine_systems() {
        // Store indices of entities to remove
        std::vector<id::id_type> to_remove;
        for (id::id_type i = 0; i < active_entities.size(); ++i) {
            if (active_entities[i]) {
                to_remove.push_back(i);
            }
        }

        // Remove stored entities
        for (auto id : to_remove) {
            remove_entity(id);
        }

        active_entities.clear();
        script::shutdown();
    }
}

EDITOR_INTERFACE id::id_type CreateGameEntity(game_entity_descriptor* e) {
    assert(e);
    game_entity_descriptor& desc{ *e };
    transform::init_info transform_info{ desc.transform.to_init_info() };
    script::init_info script_info{ desc.script.to_init_info() };
    game_entity::entity_info entity_info{
        &transform_info,
        &script_info,
    };

    auto entity = game_entity::create(entity_info);
    if (entity.is_valid()) {
        auto index = id::index(entity.get_id());
        if (index >= active_entities.size()) {
            active_entities.resize(index + 1, false);
        }
        active_entities[index] = true;
    }
    return entity.get_id();
}

EDITOR_INTERFACE void RemoveGameEntity(id::id_type id) {
    remove_entity(id);
}
*/