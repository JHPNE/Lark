#include "GameLoopAPI.h"

#define ENGINEDLL_EXPORTS

using namespace lark;

extern "C" {
    ENGINE_API bool GameLoop_Initialize(u32 target_fps, f32 fixed_timestep) {
        if (engine::g_game_loop) return false;  // Already initialized

        lark::GameLoop::Config config;
        config.target_fps = target_fps;
        config.fixed_timestep = fixed_timestep;

        engine::g_game_loop = std::make_unique<lark::GameLoop>(config);
        return engine::g_game_loop->initialize();
    }

    ENGINE_API void GameLoop_Tick() {
        if (engine::g_game_loop) {
            engine::g_game_loop->tick();  // We'll add this method to GameLoop
        }
    }

    ENGINE_API void GameLoop_Shutdown() {
        if (engine::g_game_loop) {
            engine::g_game_loop->shutdown();
            engine::g_game_loop.reset();
        }
    }

    ENGINE_API f32 GameLoop_GetDeltaTime() {
        return engine::g_game_loop ? engine::g_game_loop->get_delta_time() : 0.0f;
    }

    ENGINE_API u32 GameLoop_GetFPS() {
        return engine::g_game_loop ? engine::g_game_loop->get_fps() : 0;
    }
}