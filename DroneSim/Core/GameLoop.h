#pragma once
#include "../Common/CommonHeaders.h"
#include "../Components/Entity.h"
#include "../Components/Transform.h"
#include "../Components/Script.h"

namespace drosim {

  class GameLoop {
  public:
    struct Config {
      u32 target_fps = 60;
      f32 fixed_timestep = 1.0f/60.0f;
      bool show_fps = false;
    };

    explicit GameLoop(const Config& config);
    ~GameLoop();

    bool initialize();
    void shutdown();
    void tick();  // Process a single frame instead of running a loop

    // Getters for engine stats
    f32 get_delta_time() const { return _current_delta_time; }
    u32 get_fps() const { return _fps; }

  private:
    f32 calculate_delta_time();
    void update_transform_components(f32 dt);
    void update_script_components(f32 dt);

    Config _config;
    bool _initialized{ false };
    f32 _accumulated_time{ 0.0f };
    f32 _current_delta_time{ 0.0f };

    s64 _prev_time{ 0 };
    s64 _curr_time{ 0 };
    f64 _frequency{ 0.0 };

    u32 _frame_count{ 0 };
    u32 _fps{ 0 };
    f32 _fps_time{ 0.0f };
  };

} //