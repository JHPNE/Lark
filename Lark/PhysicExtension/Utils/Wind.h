#pragma once
#include <utility>

#include "PhysicsMath.h"

namespace lark::drones {
    using namespace physics_math;

    class Wind {
    public:
        virtual ~Wind() = default;
        virtual Eigen::Vector3f update(float t, Eigen::Vector3f position);

    protected:
        Eigen::Vector3f wind;
    };

    class NoWind : public Wind {
    public:
        NoWind() : Wind() {
            wind = Eigen::Vector3f::Zero();
        };

        Eigen::Vector3f update(float t, Eigen::Vector3f position) override {
            return wind;
        };
    };

    class ConstantWind : public Wind {
    public:
        explicit ConstantWind(Eigen::Vector3f w) : Wind() {
            wind = std::move(w);
        }

        Eigen::Vector3f update(float t, Eigen::Vector3f position) override {
            return wind;
        };
    };

    class SinusoidWind : public Wind {
    public:
        explicit SinusoidWind(Eigen::Vector3f a = Eigen::Vector3f(1, 1, 1), Eigen::Vector3f f = Eigen::Vector3f(1, 1, 1), Eigen::Vector3f p = Eigen::Vector3f::Zero()) : Wind() {
            amplitudes = std::move(a);
            frequencies = std::move(f);
            phase = std::move(p);
        }

        Eigen::Vector3f update(float t, Eigen::Vector3f position) override {
            float x = amplitudes.x() * std::sin(2.f * PI * frequencies.x() * (t + phase.x()));
            float y = amplitudes.y() * std::sin(2.f * PI * frequencies.y() * (t + phase.y()));
            float z = amplitudes.z() * std::sin(2.f * PI * frequencies.z() * (t + phase.z()));
            wind = Vector3f{x, y, z};

            return wind;
        };
    private:
        Eigen::Vector3f amplitudes;
        Eigen::Vector3f frequencies;
        Eigen::Vector3f phase;
    };

    class LadderWind : public Wind {
    public:
        explicit LadderWind(Eigen::Vector3f min = Eigen::Vector3f(-1, -1, -1), Eigen::Vector3f max = Eigen::Vector3f(1, 1, 1), Eigen::Vector3f duration = Eigen::Vector3f(1, 1, 1), Eigen::Vector3f Nstep = Eigen::Vector3f(5, 5, 5,), bool random = false) {
        }
    };
}