#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <random>

namespace lark::physics::wind {
    class IWindProfile {
    public:
        virtual ~IWindProfile() = default;
        virtual glm::vec3 update(float time, const glm::vec3& position) = 0;
    };

    class NoWind final : public IWindProfile {
        public:
        glm::vec3 update(float, const glm::vec3&) override {
            return glm::vec3(0.0f);
        };
    };

    class ConstantWind final : public IWindProfile {
        public:
        explicit ConstantWind(const glm::vec3& velocity)
            : wind_velocity(velocity) {}

        glm::vec3 update(float, const glm::vec3&) override {
            return wind_velocity;
        }

        private:
            glm::vec3 wind_velocity;
    };

    /**
     * @brief Dryden Wind Turbulence Model per MIL-F-8785C
     * @details Implements rational transfer functions that match the Dryden PSD
     */
    class DrydenGust final : public IWindProfile {
    public:
        struct Parameters {
            glm::vec3 mean_wind{10.0f, 0.0f, 0.0f};  // Vehicle velocity through air (m/s)
            float altitude{100.0f};                   // Altitude in meters
            float wingspan{2.0f};                      // Vehicle wingspan in meters

            // Turbulence intensity can be set manually or use defaults
            // Light: 0.1, Moderate: 0.4, Severe: 0.7 (multipliers)
            float turbulence_level{0.1f};
        };

        explicit DrydenGust(const Parameters& params);
        ~DrydenGust();
        glm::vec3 update(float time, const glm::vec3& position) override;

    private:
        struct FilterState {
            // For first-order filter (longitudinal)
            float y1{0.0f};

            // For second-order filter (lateral/vertical)
            float y1_2nd{0.0f};
            float y2_2nd{0.0f};
            float u1_2nd{0.0f};
        };

        Parameters params;

        // Turbulence parameters (computed from altitude)
        float L_u, L_v, L_w;    // Scale lengths (m)
        float sigma_u, sigma_v, sigma_w;  // RMS turbulence intensities (m/s)

        // Vehicle parameters
        float V;  // Vehicle speed (m/s)
        float b;  // Half wingspan (m)

        // Filter states
        FilterState filter_u;  // Longitudinal
        FilterState filter_v;  // Lateral
        FilterState filter_w;  // Vertical

        // Random number generation
        std::mt19937 rng;
        std::normal_distribution<float> white_noise;

        // Time tracking for discrete implementation
        float last_time{0.0f};

        void computeTurbulenceParameters();
        float filterFirstOrder(float input, float T, float K, FilterState& state, float dt);
        float filterSecondOrder(float input, float T, float K, FilterState& state, float dt);
    };
}