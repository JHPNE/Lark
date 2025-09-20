#pragma once
#include "PhysicExtension/Utils/PhysicsMath.h"
#include <random>
#include <vector>

namespace lark::drone
{
using namespace physics_math;

struct TrajectoryPoint
{
    // Position trajectory
    Vector3f position;     // meters
    Vector3f velocity;     // m/s  (x_dot in Python)
    Vector3f acceleration; // m/s² (x_ddot in Python)
    Vector3f jerk;         // m/s³ (x_dddot - optional, not used in basic SE3)
    Vector3f snap;         // m/s⁴ (x_ddddot - optional, not used in basic SE3)

    // Yaw trajectory
    float yaw{0};     // radians
    float yaw_dot{0}; // rad/s (yaw rate)
    float yaw_ddot{0};
};

class Trajectory
{
  public:
    virtual ~Trajectory() = default;
    virtual TrajectoryPoint update(float t) = 0;

  protected:
    TrajectoryPoint point;
};

class Circular : public Trajectory
{
  public:
    Circular(const Vector3f &center, float radius, float frequency, bool yaw_bool = false)
        : center(center), radius(radius), frequency(frequency), yaw_bool(yaw_bool)
    {
        omega = 2.0f * MATH_PI * frequency;
        omega2 = omega * omega;
        omega3 = omega2 * omega;
        omega4 = omega3 * omega;
    }

    TrajectoryPoint update(float t) override
    {
        float c = std::cos(omega * t);
        float s = std::sin(omega * t);

        // Position
        Vector3f pos(center.x() + radius * c, center.y() + radius * s, center.z() + radius * s);

        // Velocity
        Vector3f vel(-radius * omega * s, radius * omega * c, radius * omega * c);

        // acceleration
        Vector3f acc(-radius * omega2 * c, -radius * omega2 * s, -radius * omega2 * s);

        // Jerk
        Vector3f jerk(radius * omega3 * s, -radius * omega3 * c, radius * omega3 * c);

        // Snap
        Vector3f snap(radius * omega4 * c, radius * omega4 * s, radius * omega4 * s);

        // Yaw (optional, here just oscillatory if enabled)
        float yaw = 0.0f;
        float yaw_dot = 0.0f;
        float yaw_ddot = 0.0f;
        if (yaw_bool)
        {
            yaw = 0.25f * MATH_PI * std::sin(MATH_PI * t);
            yaw_dot = 0.25f * MATH_PI * MATH_PI * std::cos(MATH_PI * t);
            yaw_ddot = -0.25f * MATH_PI * MATH_PI * MATH_PI * std::sin(MATH_PI * t);
        }

        // Fill trajectory point
        TrajectoryPoint pt;
        pt.position = pos;
        pt.velocity = vel;
        pt.acceleration = acc;
        pt.jerk = jerk;
        pt.snap = snap;
        pt.yaw = yaw;
        pt.yaw_dot = yaw_dot;
        pt.yaw_ddot = yaw_ddot;

        return pt;
    };

  private:
    Vector3f center;
    float radius;
    float frequency;
    bool yaw_bool;

    float omega, omega2, omega3, omega4;
};

class Chaos : public Trajectory
{
  public:
    Chaos(const Vector3f &center, float delta, int n_points, float segment_time = 1.0f)
        : rng(std::random_device{}()), dist(-delta, delta), segment_time(segment_time)
    {

        for (int i = 0; i < n_points; ++i)
        {
            Vector3f point(center.x() + dist(rng), center.y() + dist(rng), center.z() + dist(rng));
            points.push_back(point);
        }
    };

    TrajectoryPoint update(float t) override
    {

        if (points.empty())
            return point;

        int seg_idx = static_cast<int>(t / segment_time) % (points.size() - 1);
        float alpha = fmod(t, segment_time) / segment_time;

        Vector3f p0 = points[seg_idx];
        Vector3f p1 = points[seg_idx + 1];

        // linear interpolation for now lets change to multiple different ones
        Vector3f pos = (1 - alpha) * p0 + alpha * p1;

        // velocity
        Vector3f vel = (p1 - p0) / segment_time;

        // Acceleration, jerk, snap: zero (since piecewise linear)
        Vector3f acc = Vector3f::Zero();
        Vector3f jerk = Vector3f::Zero();
        Vector3f snap = Vector3f::Zero();

        // Fill trajectory point
        point.position = pos;
        point.velocity = vel;
        point.acceleration = acc;
        point.jerk = jerk;
        point.snap = snap;

        point.yaw = 0.0f;
        point.yaw_dot = 0.0f;
        point.yaw_ddot = 0.0f;

        return point;
    };

  private:
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist;

    std::vector<Vector3f> points;
    float segment_time;
};
} // namespace lark::drones