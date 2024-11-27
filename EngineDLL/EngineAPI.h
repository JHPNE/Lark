#pragma once

#if defined(_WIN32)
    #ifdef ENGINEDLL_EXPORTS
        #define ENGINE_API __declspec(dllexport)
    #else
        #define ENGINE_API __declspec(dllimport)
    #endif
#elif defined(__APPLE__) || defined(__linux__)
    #ifdef ENGINEDLL_EXPORTS
        #define ENGINE_API __attribute__((visibility("default")))
    #else
        #define ENGINE_API
    #endif
#else
    #define ENGINE_API
    #pragma warning Unknown platform - default to no export/import
#endif


#include <Id.h>
#include <Script.h>
#include <CommonHeaders.h>
#include "EngineAPI.h"
#include <Entity.h>
#include <Transform.h>
#include <Script.h>
#include "Core/GameLoop.h"
#include <DroneSimAPI/GeometryAPI.h>

struct transform_component {
    float position[3];
    float rotation[3];
    float scale[3];
};

struct script_component {
    drosim::script::detail::script_creator script_creator;  // Use the actual type instead of void*
};

struct game_entity_descriptor {
    transform_component transform;
    script_component script;
};

namespace content_tools {
    // Match the engine's primitive types but in our own namespace
    enum class PrimitiveMeshType {
        plane,
        cube,
        uv_sphere,
        ico_sphere,
        cylinder,
        capsule,

        count
    };

    struct GeometryImportSettings {
        float smoothing_angle = 178.0f;
        uint8_t calculate_normals = 0;
        uint8_t calculate_tangents = 1;
        uint8_t reverse_handedness = 0;
        uint8_t import_embeded_textures = 1;
        uint8_t import_animations = 1;
    };

    struct SceneData {
        uint8_t* buffer = nullptr;
        uint32_t buffer_size = 0;
        drosim::tools::geometry_import_settings import_settings;
    };

    struct PrimitiveInitInfo {
        PrimitiveMeshType type;
        uint32_t segments[3] = {1, 1, 1};
        drosim::math::v3 size{1, 1, 1};
        uint32_t lod = 0;
    };
}

#ifdef __cplusplus
extern "C" {
#endif

    ENGINE_API drosim::id::id_type CreateGameEntity(game_entity_descriptor* e);
    ENGINE_API void RemoveGameEntity(drosim::id::id_type id);

    // Function to get script creator by name using engine's registration system
    ENGINE_API drosim::script::detail::script_creator GetScriptCreator(const char* name);

    // Function to get available script names
    ENGINE_API const char** GetScriptNames(size_t* count);
    ENGINE_API bool RegisterScript(const char* script_name);

    // Function to add and remove scripts from existing entities
    ENGINE_API bool AddScriptToEntity(drosim::id::id_type entity_id, const char* script_name);
    ENGINE_API bool RemoveScriptFromEntity(drosim::id::id_type entity_id);

    ENGINE_API bool GameLoop_Initialize(u32 target_fps, f32 fixed_timestep);
    ENGINE_API void GameLoop_Tick();  // Process a single frame
    ENGINE_API void GameLoop_Shutdown();
    ENGINE_API f32 GameLoop_GetDeltaTime();
    ENGINE_API u32 GameLoop_GetFPS();

    ENGINE_API bool CreatePrimitiveMesh(content_tools::SceneData* data,
                                  const content_tools::PrimitiveInitInfo* info);

    ENGINE_API bool LoadObj(const char* path, content_tools::SceneData* data);
#ifdef __cplusplus
}
#endif

namespace engine {
    void cleanup_engine_systems();
}