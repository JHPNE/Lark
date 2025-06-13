/**
 * @file GameLoop.h
 * @brief Core game loop system that manages simulation timing and updates
 * 
 * The GameLoop class is responsible for managing the main simulation loop,
 * handling both fixed and variable timestep updates, and maintaining
 * consistent frame timing.
 */

#pragma once
#include "../Common/CommonHeaders.h"
#include "../Components/Entity.h"
#include "../Components/Transform.h"
#include "../Components/Script.h"

namespace lark {

  /**
   * @class GameLoop
   * @brief Main simulation loop manager
   * 
   * Handles the core update loop of the simulation, managing:
   * - Fixed timestep updates for physics and transformations
   * - Variable timestep updates for scripts
   * - FPS monitoring and control
   * - Delta time calculations
   */
  class GameLoop {
  public:
    /**
     * @struct Config
     * @brief Configuration settings for the game loop
     */
    struct Config {
      u32 target_fps = 60;      ///< Target frames per second
      f32 fixed_timestep = 1.0f/60.0f;  ///< Fixed timestep for physics updates (in seconds)
      bool show_fps = false;    ///< Whether to display FPS counter
    };

    /**
     * @brief Constructs a GameLoop with the specified configuration
     * @param config Configuration settings for the game loop
     */
    explicit GameLoop(const Config& config);
    
    /**
     * @brief Destructor that ensures proper cleanup
     */
    ~GameLoop();

    /**
     * @brief Initializes the game loop
     * @return true if initialization was successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Shuts down the game loop and cleans up resources
     */
    void shutdown();

    /**
     * @brief Processes a single frame of the simulation
     * 
     * This method:
     * 1. Calculates delta time
     * 2. Accumulates time for fixed updates
     * 3. Processes fixed timestep updates
     * 4. Processes variable timestep updates
     * 5. Updates FPS counter
     */
    void tick();

    /**
     * @brief Gets the time elapsed since last frame
     * @return Delta time in seconds
     */
    f32 get_delta_time() const { return _current_delta_time; }

    /**
     * @brief Gets the current frames per second
     * @return Current FPS
     */
    u32 get_fps() const { return _fps; }

  private:
    /**
     * @brief Calculates time between frames
     * @return Time elapsed since last frame in seconds
     */
    f32 calculate_delta_time();

    /**
     * @brief Updates transform components with fixed timestep
     * @param dt Fixed timestep duration
     */
    void update_transform_components(f32 dt);

    /**
     * @brief Updates script components with variable timestep
     * @param dt Variable timestep duration
     */
    void update_script_components(f32 dt);

    Config _config;                 ///< Game loop configuration
    bool _initialized{ false };     ///< Initialization state
    f32 _accumulated_time{ 0.0f };  ///< Accumulated time for fixed updates
    f32 _current_delta_time{ 0.0f };///< Current frame delta time

    s64 _prev_time{ 0 };           ///< Previous frame timestamp
    s64 _curr_time{ 0 };           ///< Current frame timestamp
    f64 _frequency{ 0.0 };         ///< Timer frequency

    u32 _frame_count{ 0 };         ///< Frames counted for FPS calculation
    u32 _fps{ 0 };                 ///< Current FPS value
    f32 _fps_time{ 0.0f };         ///< Time accumulated for FPS calculation
  };

} // namespace lark