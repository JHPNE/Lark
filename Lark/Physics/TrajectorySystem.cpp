#include "TrajectorySystem.h"
#include <algorithm>

#include "Utils/MathTypes.h"

namespace lark::physics::trajectory {
    drones::FlatOutput CircularTrajectory::update(float time) {
        float omega = 2.0f * math::pi * params.frequency;
        float theta = omega * time;

        drones::FlatOutput output;

        // Position
        output.position = params.center + glm::vec3(
            params.radius * std::cos(theta),
            params.radius * std::sin(theta),
            params.height
        );

        // Velocity
        output.velocity = glm::vec3(
            -params.radius * omega * std::sin(theta),
            params.radius * omega * std::cos(theta),
            0.0f
        );

        // Acceleration
        output.acceleration = glm::vec3(
            -params.radius * omega * omega * std::cos(theta),
            -params.radius * omega * omega * std::sin(theta),
            0.0f
        );

        // Jerk
        output.jerk = glm::vec3(
            params.radius * omega * omega * omega * std::sin(theta),
            -params.radius * omega * omega * omega * std::cos(theta),
            0.0f
        );

        // Snap
        output.snap = glm::vec3(
            params.radius * omega * omega * omega * omega * std::cos(theta),
            params.radius * omega * omega * omega * omega * std::sin(theta),
            0.0f
        );

        // Yaw
        if (params.yaw_follows_velocity) {
            output.yaw = std::atan2(output.velocity.y, output.velocity.x);
            output.yawRate = omega;
        } else {
            output.yaw = 0.0f;
            output.yawRate = 0.0f;
        }

        return output;
    }

    MinSnapTrajectory::MinSnapTrajectory(const std::vector<Waypoint>& waypoints) {
        computeTrajectory(waypoints);
    }

    void MinSnapTrajectory::computeTrajectory(const std::vector<Waypoint>& waypoints) {
        if (waypoints.size() < 2) {
            throw std::invalid_argument("Need at least 2 waypoints");
        }

        segments.clear();
        segments.reserve(waypoints.size() - 1);

        for (size_t i = 0; i < waypoints.size() - 1; ++i) {
            const auto& wp1 = waypoints[i];
            const auto& wp2 = waypoints[i + 1];

            float dt = wp2.time - wp1.time;
            if (dt <= 0) {
                throw std::invalid_argument("Invalid waypoint times");
            }

            // For minimum snap, we need 8 constraints per segment:
            // - Position at start and end (2)
            // - Velocity at start and end (2)
            // - Acceleration at start and end (2)
            // - Jerk at start and end (2)
            constexpr size_t N = 8;
            std::array<std::array<float, N>, N> A{};
            std::array<std::array<float, 3>, N> b_pos{};  // 3 columns for x,y,z
            std::array<float, N> b_yaw{};

            for (int j = 0; j < N; ++j) {
                A[0][j] = (j == 0) ? 1.0f : 0.0f;  // pow(0, j) for j>0 is 0
                A[1][j] = std::pow(dt, j);
            }

            // Velocity constraints at t=0 and t=dt
            A[2][0] = 0.0f;  // No constant term in derivative
            A[3][0] = 0.0f;
            for (int j = 1; j < N; ++j) {
                A[2][j] = (j == 1) ? 1.0f : 0.0f;  // j * pow(0, j-1) at t=0
                A[3][j] = j * std::pow(dt, j-1);    // j * pow(dt, j-1) at t=dt
            }

            // Acceleration constraints at t=0 and t=dt
            A[4][0] = A[4][1] = 0.0f;
            A[5][0] = A[5][1] = 0.0f;
            for (int j = 2; j < N; ++j) {
                A[4][j] = (j == 2) ? 2.0f : 0.0f;  // j*(j-1) * pow(0, j-2) at t=0
                A[5][j] = j * (j-1) * std::pow(dt, j-2);
            }

            // Jerk constraints at t=0 and t=dt
            for (int j = 0; j < 3; ++j) {
                A[6][j] = 0.0f;
                A[7][j] = 0.0f;
            }
            for (int j = 3; j < N; ++j) {
                A[6][j] = (j == 3) ? 6.0f : 0.0f;  // j*(j-1)*(j-2) * pow(0, j-3) at t=0
                A[7][j] = j * (j-1) * (j-2) * std::pow(dt, j-3);
            }


            // Set boundary conditions for position
            b_pos[0][0] = wp1.position.x;
            b_pos[0][1] = wp1.position.y;
            b_pos[0][2] = wp1.position.z;
            b_pos[1][0] = wp2.position.x;
            b_pos[1][1] = wp2.position.y;
            b_pos[1][2] = wp2.position.z;

            // Zero velocity/acceleration/jerk at trajectory endpoints
            if (i == 0) {
                // Start of trajectory
                b_pos[2][0] = b_pos[2][1] = b_pos[2][2] = 0.0f;  // vel
                b_pos[4][0] = b_pos[4][1] = b_pos[4][2] = 0.0f;  // accel
                b_pos[6][0] = b_pos[6][1] = b_pos[6][2] = 0.0f;  // jerk
            } else {
                // Continuity constraints (handled separately between segments)
                b_pos[2][0] = b_pos[2][1] = b_pos[2][2] = 0.0f;
                b_pos[4][0] = b_pos[4][1] = b_pos[4][2] = 0.0f;
                b_pos[6][0] = b_pos[6][1] = b_pos[6][2] = 0.0f;
            }

            if (i == waypoints.size() - 2) {
                // End of trajectory
                b_pos[3][0] = b_pos[3][1] = b_pos[3][2] = 0.0f;  // vel
                b_pos[5][0] = b_pos[5][1] = b_pos[5][2] = 0.0f;  // accel
                b_pos[7][0] = b_pos[7][1] = b_pos[7][2] = 0.0f;  // jerk
            } else {
                // Continuity constraints (handled separately between segments)
                b_pos[3][0] = b_pos[3][1] = b_pos[3][2] = 0.0f;
                b_pos[5][0] = b_pos[5][1] = b_pos[5][2] = 0.0f;
                b_pos[7][0] = b_pos[7][1] = b_pos[7][2] = 0.0f;
            }

            // Yaw boundary conditions
            b_yaw[0] = wp1.yaw;
            b_yaw[1] = wp2.yaw;
            for (int j = 2; j < N; ++j) {
                b_yaw[j] = 0.0f;
            }

            // Solve for coefficients
            auto coeffs_pos = math::solveMultiple<N, 3>(A, b_pos);
            auto coeffs_yaw = math::solve<N>(A, b_yaw);

            // Store segment
            Segment seg;
            seg.start_time = wp1.time;
            seg.duration = dt;

            for (int j = 0; j < N; ++j) {
                seg.position_coeffs[j] = glm::vec3(
                    coeffs_pos[j][0],
                    coeffs_pos[j][1],
                    coeffs_pos[j][2]
                );
                seg.yaw_coeffs[j] = coeffs_yaw[j];
            }

            segments.push_back(seg);
        }

        total_duration = waypoints.back().time;
    }

    glm::vec3 MinSnapTrajectory::evaluatePolynomial(
    const std::array<glm::vec3, 8>& coeffs, float t, int derivative) {

        glm::vec3 result(0.0f);

        for (int i = derivative; i < 8; ++i) {
            float coeff = 1.0f;
            for (int j = 0; j < derivative; ++j) {
                coeff *= (i - j);
            }
            result += coeff * coeffs[i] * std::pow(t, i - derivative);
        }

        return result;
    }

    drones::FlatOutput MinSnapTrajectory::update(float time) {
        drones::FlatOutput output;

        // Clamp time to trajectory duration
        time = glm::clamp(time, 0.0f, total_duration);

        // Find active segment
        const Segment* active_segment = nullptr;
        float local_time = 0.0f;

        for (const auto& seg : segments) {
            if (time >= seg.start_time && time <= seg.start_time + seg.duration) {
                active_segment = &seg;
                local_time = time - seg.start_time;
                break;
            }
        }

        if (!active_segment) {
            // Return last position if beyond trajectory
            active_segment = &segments.back();
            local_time = active_segment->duration;
        }

        // Evaluate polynomials
        output.position = evaluatePolynomial(active_segment->position_coeffs, local_time, 0);
        output.velocity = evaluatePolynomial(active_segment->position_coeffs, local_time, 1);
        output.acceleration = evaluatePolynomial(active_segment->position_coeffs, local_time, 2);
        output.jerk = evaluatePolynomial(active_segment->position_coeffs, local_time, 3);
        output.snap = evaluatePolynomial(active_segment->position_coeffs, local_time, 4);

        // Evaluate yaw
        float yaw_val = 0.0f;
        float yaw_rate = 0.0f;
        for (int i = 0; i < 8; ++i) {
            yaw_val += active_segment->yaw_coeffs[i] * std::pow(local_time, i);
            if (i > 0) {
                yaw_rate += i * active_segment->yaw_coeffs[i] * std::pow(local_time, i-1);
            }
        }

        output.yaw = yaw_val;
        output.yawRate = yaw_rate;

        return output;
    }
}
