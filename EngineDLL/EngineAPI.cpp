#include "EngineAPI.h"

#define ENGINEDLL_EXPORTS

using namespace drosim;

namespace {
    std::unique_ptr<GameLoop> g_game_loop;

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

    // Create a concrete script class for each script
    class PythonScriptWrapper : public drosim::script::entity_script {
    public:
        explicit PythonScriptWrapper(drosim::game_entity::entity entity)
            : entity_script(entity) {}
    };

    // Convert from ContentTools types to Engine types
    drosim::tools::primitive_mesh_type ConvertPrimitiveType(content_tools::PrimitiveMeshType type) {
        return static_cast<drosim::tools::primitive_mesh_type>(type);
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

    ENGINE_API script::detail::script_creator GetScriptCreator(const char* name) {
        if (!name) return nullptr;
        // Use engine's existing script registry system
        return script::detail::get_script_creator(
            script::detail::string_hash()(name));
    }

    ENGINE_API const char** GetScriptNames(size_t* count) {
        // First get the count of script names
        size_t name_count = 0;
        script::detail::get_script_names(nullptr, &name_count);

        if (name_count == 0) {
            *count = 0;
            return nullptr;
        }

        // Resize the static vector to hold all name pointers
        static std::vector<const char*> name_ptrs;
        name_ptrs.resize(name_count);

        // Now get the actual names
        script::detail::get_script_names(name_ptrs.data(), &name_count);
        *count = name_count;

        return name_ptrs.data();
    }

    ENGINE_API bool RegisterScript(const char* script_name) {
        if (!script_name) return false;

        auto creator = [](drosim::game_entity::entity entity) -> drosim::script::detail::script_ptr {
            return std::make_unique<PythonScriptWrapper>(entity);
        };

        bool success = drosim::script::detail::register_script(
            drosim::script::detail::string_hash()(script_name),
            creator);

        if (success) {
            drosim::script::detail::add_script_name(script_name);
        }

        return success;
    }

    ENGINE_API bool GameLoop_Initialize(u32 target_fps, f32 fixed_timestep) {
        if (g_game_loop) return false;  // Already initialized

        drosim::GameLoop::Config config;
        config.target_fps = target_fps;
        config.fixed_timestep = fixed_timestep;

        g_game_loop = std::make_unique<drosim::GameLoop>(config);
        return g_game_loop->initialize();
    }

    ENGINE_API void GameLoop_Tick() {
        if (g_game_loop) {
            g_game_loop->tick();  // We'll add this method to GameLoop
        }
    }

    ENGINE_API void GameLoop_Shutdown() {
        if (g_game_loop) {
            g_game_loop->shutdown();
            g_game_loop.reset();
        }
    }

    ENGINE_API f32 GameLoop_GetDeltaTime() {
        return g_game_loop ? g_game_loop->get_delta_time() : 0.0f;
    }

    ENGINE_API u32 GameLoop_GetFPS() {
        return g_game_loop ? g_game_loop->get_fps() : 0;
    }

    ENGINE_API bool CreatePrimitiveMesh(content_tools::SceneData* data,
                                  const content_tools::PrimitiveInitInfo* info) {
        if (!data || !info) {
            printf("[CreatePrimitiveMesh] Error: Invalid parameters\n");
            return false;
        }

        auto engine_data = drosim::tools::scene_data{};
        auto engine_info = drosim::tools::primitive_init_info{};

        // Convert info to engine format
        engine_info.mesh_type = static_cast<drosim::tools::primitive_mesh_type>(info->type);
        memcpy(engine_info.segments, info->segments, sizeof(info->segments));
        engine_info.size = info->size;
        engine_info.lod = info->lod;

        engine_data.settings = data->import_settings;

        // Let the engine create the mesh
        printf("[CreatePrimitiveMesh] Calling engine CreatePrimitiveMesh\n");
        drosim::tools::CreatePrimitiveMesh(&engine_data, &engine_info);

        // Just pass through the engine's buffer
        if (engine_data.buffer && engine_data.buffer_size > 0) {
            printf("[CreatePrimitiveMesh] Engine created buffer of size: %u\n", engine_data.buffer_size);
            data->buffer = engine_data.buffer;
            data->buffer_size = engine_data.buffer_size;
            return true;
        }

        printf("[CreatePrimitiveMesh] Failed to create buffer\n");
        return false;
    }
}