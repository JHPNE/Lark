#pragma once
#include <random>
#include <__random/random_device.h>

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
        explicit LadderWind(Eigen::Vector3f min = Eigen::Vector3f(-1, -1, -1),
                            Eigen::Vector3f max = Eigen::Vector3f(1, 1, 1),
                            Eigen::Vector3f d = Eigen::Vector3f(1, 1, 1),
                            Eigen::Vector3f Nstep = Eigen::Vector3f(5, 5, 5),
                            bool r = false)
                                : duration(d), random(r), gen(std::random_device{}()) {
            // Input validation
            if (Nstep.x() <= 0 || Nstep.y() <= 0 || Nstep.z() <= 0) {
                throw std::invalid_argument("LadderWind Error: The number of steps must be greater than 0");
            }
            // Store step counts
            nx = static_cast<int>(Nstep.x());
            ny = static_cast<int>(Nstep.y());
            nz = static_cast<int>(Nstep.z());

            // Create arrays
            wx_arr = Eigen::VectorXf::LinSpaced(nx, min.x(), max.x());
            wy_arr = Eigen::VectorXf::LinSpaced(ny, min.y(), max.y());
            wz_arr = Eigen::VectorXf::LinSpaced(nz, min.z(), max.z());

            if (random) {
                std::uniform_int_distribution<> distx(0, nx - 1);
                std::uniform_int_distribution<> disty(0, ny - 1);
                std::uniform_int_distribution<> distz(0, nz - 1);

                xid = distx(gen);
                yid = disty(gen);
                zid = distz(gen);
            } else {
                xid = yid = zid = 0;
            }

            timer = Vector3f(-1.f, -1.f, -1.f);

            wx = wx_arr[xid];
            wy = wy_arr[yid];
            wz = wz_arr[zid];
        }

        Eigen::Vector3f update(float t, Eigen::Vector3f position) override {

            if (timer.x() == -1.f) {
                timer = Vector3f(t, t, t);
            }

            if ((t - timer.x()) >= duration.x()) {
                if (random) {
                    std::uniform_int_distribution<> distx(0, nx - 1);
                    xid = distx(gen);
                } else {
                    xid = (xid + 1) % nx;
                }
                wx = wx_arr[xid];
                timer.x() = t;
            }

            if ((t - timer.y()) >= duration.y()) {
                if (random) {
                    std::uniform_int_distribution<> disty(0, ny - 1);
                    yid = disty(gen);
                } else {
                    yid = (yid + 1) % ny;
                }
                wy = wy_arr[yid];
                timer.y() = t;
            }

            if ((t - timer.z()) >= duration.z()) {
                if (random) {
                    std::uniform_int_distribution<> distz(0, nz - 1);
                    zid = distz(gen);
                } else {
                    zid = (zid + 1) % nz;
                }
                wz = wz_arr[zid];
                timer.z() = t;
            }

            return {wx, wy, wz};
        }

    private:
        int xid, yid, zid;
        int nx, ny, nz;
        Eigen::VectorXf wx_arr, wy_arr, wz_arr;
        float wx, wy, wz;
        Eigen::Vector3f duration;
        Eigen::Vector3f timer;
        bool random;
        std::mt19937 gen;
    };
}