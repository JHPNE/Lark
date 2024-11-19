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


#ifdef __cplusplus
}
#endif

namespace engine {
    void cleanup_engine_systems();
}