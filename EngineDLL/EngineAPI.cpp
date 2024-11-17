#include <CommonHeaders.h>
#include <Entity.h>
#include <Transform.h>
#include <Script.h>
#include "EngineAPI.h"

#define ENGINEDLL_EXPORTS

using namespace drosim;

namespace {
    // Internal implementation of transform conversion
    transform::init_info to_engine_transform(const transform_component& transform) {
        transform::init_info info{};
        memcpy(&info.position[0], &transform.position[0], sizeof(float) * 3);
        memcpy(&info.scale[0], &transform.scale[0], sizeof(float) * 3);

        glm::vec3 euler_angles(transform.rotation[0], transform.rotation[1], transform.rotation[2]);
        glm::quat quaternion = glm::quat(euler_angles);

        info.rotation[0] = quaternion.x;
        info.rotation[1] = quaternion.y;
        info.rotation[2] = quaternion.z;
        info.rotation[3] = quaternion.w;

        return info;
    }

    // Internal implementation of script conversion
    script::init_info to_engine_script(const script_component& script) {
        script::init_info info{};
        info.script_creator = script.script_creator;  // Direct assignment since types match
        return info;
    }

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
            if (auto script_comp = entity.script(); script_comp.is_valid()) {
                script::remove(script_comp);
            }
            game_entity::remove(entity_id);
        }

        const auto index = id::index(id);
        active_entities[index] = false;
    }
}

namespace engine {
    void cleanup_engine_systems() {
        std::vector<id::id_type> to_remove;
        for (id::id_type i = 0; i < active_entities.size(); ++i) {
            if (active_entities[i]) {
                to_remove.push_back(i);
            }
        }

        for (auto id : to_remove) {
            remove_entity(id);
        }

        active_entities.clear();
        script::shutdown();
    }
}

extern "C" {
    ENGINE_API id::id_type CreateGameEntity(game_entity_descriptor* e) {
        assert(e);
        transform::init_info transform_info = to_engine_transform(e->transform);
        script::init_info script_info = to_engine_script(e->script);

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

    ENGINE_API void RemoveGameEntity(id::id_type id) {
        remove_entity(id);
    }

    ENGINE_API script_component CreateScriptComponent(drosim::id::id_type id) {
        script::init_info info{};
    }
}