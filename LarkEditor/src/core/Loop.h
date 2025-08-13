#pragma once
#include "EngineAPI.h"
#include <iostream>  // for error reporting if needed
#include <thread>

class Loop {
public:
    static bool Initialize() {
        _running = false;
        target_fps = 60;
        fixed_time_step = 1.0f / target_fps;
        return true;
    }

    static void Run() {
        if (!Initialize()) {
            return;  // Failed to initialize
        }

        _running = true;  // Set running before entering the loop

        if (GameLoop_Initialize(target_fps, fixed_time_step)) {
            while (_running) {
                GameLoop_Tick();

                float deltaTime = GameLoop_GetDeltaTime();
                unsigned int FPS = GameLoop_GetFPS();
            }
        }

        GameLoop_Shutdown();
    }

    static void StartAsync() {
        if (_loopThread.joinable()) return;
        _loopThread = std::thread([] {
            Run();
        });
    }

    static void Stop() {
        _running = false;  // Signal the loop to stop
        if (_loopThread.joinable()) _loopThread.join();
    }

    static bool SetRunning(bool value) {
        _running = value;
        return _running;
    }

private:
    static inline bool _running = false;
    static inline int target_fps = 60;
    static inline float fixed_time_step = 1.0f / target_fps;
    static inline std::thread _loopThread;
};
