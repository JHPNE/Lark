#include "GameLoop.h"

#if defined(_WIN32)
#include <Windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <time.h>
#endif

namespace drosim {

    namespace {
#if defined(_WIN32)
        f64 get_frequency() {
            LARGE_INTEGER freq;
            QueryPerformanceFrequency(&freq);
            return static_cast<f64>(freq.QuadPart);
        }

        s64 get_timer_value() {
            LARGE_INTEGER value;
            QueryPerformanceCounter(&value);
            return value.QuadPart;
        }
#else
        f64 get_frequency() { return 1000000000.0; } // Nanoseconds

        s64 get_timer_value() {
            timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            return static_cast<s64>(ts.tv_sec) * 1000000000LL + ts.tv_nsec;
        }
#endif
    }

    GameLoop::GameLoop(const Config& config)
        : _config(config)
        , _frequency(get_frequency())
    {}

    GameLoop::~GameLoop() { shutdown();}

    bool GameLoop::initialize() {
        if (_initialized) return false;

        _prev_time = get_timer_value();
        _initialized = true;

        return true;
    }

    void GameLoop::shutdown() {
        if (!_initialized) return;

        //engine::cleanup_engine_systems();
        _initialized = false;
    }

    void GameLoop::tick() {
        if (!_initialized) return;

        _current_delta_time = calculate_delta_time();
        _accumulated_time += _current_delta_time;

        // Fixed timestep updates
        while (_accumulated_time >= _config.fixed_timestep) {
            update_transform_components(_config.fixed_timestep);
            _accumulated_time -= _config.fixed_timestep;
        }

        // Variable timestep updates
        update_script_components(_current_delta_time);

        // Update FPS counter
        _frame_count++;
        _fps_time += _current_delta_time;

        if (_fps_time >= 1.0f) {
            _fps = _frame_count;
            _frame_count = 0;
            _fps_time -= 1.0f;

            if (_config.show_fps) {
                printf("FPS: %u\n", _fps);
            }
        }
    }

    f32 GameLoop::calculate_delta_time() {
      _curr_time = get_timer_value();
      const f32 delta_time = static_cast<f32>(_curr_time - _prev_time) / static_cast<f32>(_frequency);
      _prev_time = _curr_time;

      if (_config.show_fps) {
        _frame_count++;
        _fps_time += delta_time;

        if (_fps_time >= 1.0f) {
          printf("FPS: %u\n", _frame_count);
          _frame_count = 0;
          _fps_time -= 1.0f;
        }
      }

      return delta_time;
    }


    void GameLoop::update_transform_components(f32 dt) {
        const auto& active_entities = game_entity::get_active_entities();
        for (const auto& entity_id : active_entities) {
            game_entity::entity entity{ entity_id };
            // No need to check is_alive() since we maintain the active list
            auto transform = entity.transform();
            if (transform.is_valid()) {
                // Update transform
            }
        }
    }

    void GameLoop::update_script_components(f32 dt) {
        const auto& active_entities = game_entity::get_active_entities();
        for (const auto& entity_id : active_entities) {
            game_entity::entity entity{ entity_id };
            auto script = entity.script();
            if (script.is_valid()) {
                // Update script
            }
        }
    }

} //namespace drosim