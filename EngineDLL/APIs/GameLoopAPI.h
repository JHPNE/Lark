#pragma once
#include "EngineCoreAPI.h"

#ifdef __cplusplus
extern "C" {
#endif

    ENGINE_API bool GameLoop_Initialize(u32 target_fps, f32 fixed_timestep);
    ENGINE_API void GameLoop_Tick();
    ENGINE_API void GameLoop_Shutdown();
    ENGINE_API f32 GameLoop_GetDeltaTime();
    ENGINE_API u32 GameLoop_GetFPS();

#ifdef __cplusplus
}
#endif
