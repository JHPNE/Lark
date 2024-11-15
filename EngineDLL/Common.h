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

namespace engine {
    void cleanup_engine_systems();
}
