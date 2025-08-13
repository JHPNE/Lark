#pragma once
#include "ControllerTypes.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <array>

namespace lark::physics::trajectory {

    class ITrajectory {
    public:
        virtual ~ITrajectory() = default;
        virtual drones::FlatOutput update(float time) = 0;
        virtual float getDuration() const = 0;
        virtual bool isComplete(float time) const {
            return time >= getDuration();
        }
    };

    class HoverTrajectory final : public ITrajectory {
    public:
        explicit HoverTrajectory(const glm::vec3& position, float yaw = 0.0f)
            : hover_position(position), hover_yaw(yaw) {}

        drones::FlatOutput update(float) override {
            return {
                hover_position,
                glm::vec3(0.0f),
                glm::vec3(0.0f),
                glm::vec3(0.0f),
                glm::vec3(0.0f),
                hover_yaw,
                0.0f
            };
        }

        float getDuration() const override { return std::numeric_limits<float>::max(); }

    private:
        glm::vec3 hover_position;
        float hover_yaw;
    };

    class MinSnapTrajectory final : public ITrajectory {
    public:
        struct Waypoint {
            glm::vec3 position;
            float yaw;
            float time;
        };

        explicit MinSnapTrajectory(const std::vector<Waypoint>& waypoints);
        drones::FlatOutput update(float time) override;
        float getDuration() const override { return total_duration; }

    private:
        struct Segment {
            std::array<glm::vec3, 8> position_coeffs;  // 7th order polynomial
            std::array<float, 8> yaw_coeffs;
            float start_time;
            float duration;
        };

        std::vector<Segment> segments;
        float total_duration;

        void computeTrajectory(const std::vector<Waypoint>& waypoints);
        glm::vec3 evaluatePolynomial(const std::array<glm::vec3, 8>& coeffs, float t, int derivative = 0);
    };

    class CircularTrajectory final : public ITrajectory {
    public:
        struct Parameters {
            glm::vec3 center{0.0f};
            float radius{1.0f};
            float height{1.0f};
            float frequency{0.2f};
            bool yaw_follows_velocity{true};
        };

        explicit CircularTrajectory(const Parameters& params)
            : params(params) {}

        drones::FlatOutput update(float time) override;
        float getDuration() const override { return std::numeric_limits<float>::max(); }

    private:
        Parameters params;
    };

}