// Lark/Physics/Environment.h
#pragma once
#include <glm/glm.hpp>
#include "Wind.h"
#include <memory>

namespace lark::physics {
    struct Settings {
        glm::vec3 gravity{0.0f, 0.0f, -9.81f};
        float air_density{1.225f};
        bool enable_collisions{true};
        float safety_margin{0.25f};
    };
    /**
     * @brief Global physics settings and world state
     * Singleton pattern for world-wide physics parameters
     */
    class Environment {
    public:
        static Environment& getInstance() {
            static Environment instance;
            return instance;
        }

        void initialize(const Settings& settings = Settings()) {
            settings_ = settings;
            wind_profile_ = std::make_shared<wind::NoWind>();
            initialized_ = true;
        }

        void shutdown() {
            initialized_ = false;
            wind_profile_ = std::make_shared<wind::NoWind>();
        }

        void setWindProfile(std::shared_ptr<wind::IWindProfile> wind) {
            wind_profile_ = wind;
        }

        glm::vec3 getWindAt(float time, const glm::vec3& position) const {
            if (wind_profile_) {
                return wind_profile_->update(time, position);
            }
            return glm::vec3(0.0f);
        }

        const Settings& getSettings() const { return settings_; }
        bool isInitialized() const { return initialized_; }

    private:
        Environment() = default;
        ~Environment() = default;
        Environment(const Environment&) = delete;
        Environment& operator=(const Environment&) = delete;

        Settings settings_;
        std::shared_ptr<wind::IWindProfile> wind_profile_;
        bool initialized_{false};
    };
}