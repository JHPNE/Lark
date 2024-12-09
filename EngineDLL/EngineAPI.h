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

#include "Core/GameLoop.h"
#include "EngineAPI.h"
#include <CommonHeaders.h>
#include <DroneSimAPI/GeometryAPI.h>
#include <Entity.h>
#include <Id.h>
#include <Script.h>
#include <Transform.h>
#include <Geometry.h>
#include "Structures.h"

#ifdef __cplusplus
extern "C" {
#endif

    ENGINE_API drosim::id::id_type CreateGameEntity(game_entity_descriptor* e);
    ENGINE_API bool RemoveGameEntity(drosim::id::id_type id);

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

    // GEOMETRY API CALLS
    ENGINE_API bool CreatePrimitiveMesh(content_tools::SceneData* data,
                                  const content_tools::PrimitiveInitInfo* info);

    ENGINE_API bool LoadObj(const char* path, content_tools::SceneData* data);

    // Transform API CALLS
    ENGINE_API bool SetEntityTransform(drosim::id::id_type entity_id, const transform_component& transform);
    ENGINE_API bool GetEntityTransform(drosim::id::id_type entity_id, transform_component* out_transform);
    ENGINE_API bool ResetEntityTransform(drosim::id::id_type entity_id);
    ENGINE_API glm::mat4 GetEntityTransformMatrix(drosim::id::id_type entity_id);

    // Function to modify vertex positions of an entity's geometry
    ENGINE_API bool ModifyEntityVertexPositions(drosim::id::id_type entity_id, std::vector<glm::vec3>& new_positions);

#ifdef __cplusplus
}
#endif

namespace engine {
    void cleanup_engine_systems();
}