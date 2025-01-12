#include "Rotor.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <iostream> // For debugging outputs

namespace lark::rotor {
    namespace {
        struct rotor_data : drone_components::component_data<drone_data::RotorBody> {
            float current_thrust{0.0f};
            static constexpr int NUM_BLADE_ELEMENTS = 10;
        };

        drone_components::component_pool<rotor_id, rotor_data> pool;
        constexpr float MAX_RPM = 15000.0f;

        struct BladeElement {
            float radius;      // Local radius (m)
            float chord;       // Local chord length (m)
            float twist;       // Local geometric twist (rad)
            float dr;         // Element width (m)
            float local_vel;  // Local velocity (m/s)
            float local_aoa;  // Local angle of attack (rad)
        };

        float calculate_tip_loss(float r, float R, int B, float phi) {
            if (phi <= 0.0f) return 1.0f; // Prevent division by zero
            const float f = (B / 2.0f) * ((R - r) / (r * std::sin(phi)));
            return (2.0f / glm::pi<float>()) * std::acos(std::exp(-f));
        }

        std::pair<float, float> get_airfoil_coefficients(float alpha, float reynolds) {
            const float alpha_deg = alpha * 180.0f / glm::pi<float>();

            // Enhanced lift coefficient calculation
            float cl = 6.2f * std::sin(alpha);  // Modified from 2π to better match real propeller data

            // Realistic stall behavior
            const float stall_angle = 12.0f;  // Degrees
            if (std::abs(alpha_deg) > stall_angle) {
                float post_stall = 0.9f * (1.0f - std::min(1.0f, (std::abs(alpha_deg) - stall_angle) / 15.0f));
                cl *= post_stall;
            }

            // Reynolds number effects
            float re_factor = std::min(1.0f, std::max(0.7f, reynolds / 70000.0f));
            cl *= re_factor;

            // Profile drag with realistic coefficients
            float cd = 0.015f;  // Base profile drag
            cd += 0.015f * std::abs(alpha);  // Angle of attack dependent drag
            cd += 0.01f * (1.0f - re_factor);  // Reynolds number effects

            return {cl, cd};
        }

        void calculate_blade_element_conditions(BladeElement& element,
                                                const rotor_data* data,
                                                float omega,
                                                const btVector3& local_velocity) {
            float Ut = omega * element.radius;
            float Up = local_velocity.dot(data->rotorNormal);

            // Include induced velocity estimate
            float thrust_coeff = data->current_thrust / (data->airDensity * data->discArea * std::pow(omega * data->bladeRadius, 2));
            float induced_vel = std::sqrt(thrust_coeff / 2.0f);
            Up += induced_vel;

            float phi = std::atan2(Up, Ut);
            element.local_aoa = element.twist - phi;
            element.local_vel = std::sqrt(Ut * Ut + Up * Up);
        }

        std::pair<float, float> calculate_element_forces(const BladeElement& element,
                                                         const rotor_data* data,
                                                         float tip_loss) {
            float reynolds = (data->airDensity * element.local_vel * element.chord) / 1.81e-5f;
            auto [cl, cd] = get_airfoil_coefficients(element.local_aoa, reynolds);

            cl *= tip_loss;
            cd *= tip_loss;

            float q = 0.5f * data->airDensity * element.local_vel * element.local_vel;
            float dL = cl * q * element.chord * element.dr;
            float dD = cd * q * element.chord * element.dr;

            // Convert lift and drag to normal and tangential forces
            float phi = std::atan2(dD, dL);
            float dN = dL * std::cos(phi) + dD * std::sin(phi);
            float dT = dL * std::sin(phi) - dD * std::cos(phi);

            return {dN, dT};
        }

        std::pair<float, float> calculate_bem_forces(const rotor_data* data,
                                                     const btVector3& local_velocity) {
            if (!data || data->currentRPM <= 0.0f) return {0.0f, 0.0f};

            const float omega = (data->currentRPM * 2.0f * glm::pi<float>()) / 60.0f;
            std::vector<BladeElement> elements(rotor_data::NUM_BLADE_ELEMENTS);

            // Initialize blade elements with realistic geometry
            const float dr = data->bladeRadius / rotor_data::NUM_BLADE_ELEMENTS;
            const float root_chord = 0.15f * data->bladeRadius;  // Wider chord
            const float tip_chord = 0.08f * data->bladeRadius;   // Narrower at tip
            const float root_twist = data->bladePitch + 0.175f;  // Additional twist at root
            const float tip_twist = data->bladePitch;            // Less twist at tip

            for (int i = 0; i < rotor_data::NUM_BLADE_ELEMENTS; i++) {
                float r_ratio = float(i + 0.5f) / rotor_data::NUM_BLADE_ELEMENTS;
                elements[i].radius = r_ratio * data->bladeRadius;
                elements[i].dr = dr;
                // Linear taper from root to tip
                elements[i].chord = root_chord + (tip_chord - root_chord) * r_ratio;
                // Linear twist distribution
                elements[i].twist = root_twist + (tip_twist - root_twist) * r_ratio;
            }

            float total_thrust = 0.0f;
            float total_torque = 0.0f;

            for (auto& element : elements) {
                calculate_blade_element_conditions(element, data, omega, local_velocity);

                float phi = std::atan2(local_velocity.dot(data->rotorNormal), omega * element.radius);
                float F = calculate_tip_loss(element.radius, data->bladeRadius, data->bladeCount, phi);

                auto [dN, dT] = calculate_element_forces(element, data, F);

                dN *= data->bladeCount;
                dT *= data->bladeCount;

                total_thrust += dN;
                total_torque += dT * element.radius;
            }

            return {total_thrust, total_torque};
        }
    }

    void drone_component::calculate_forces(float deltaTime) {
        auto* data = pool.get_data(get_id());
        if (!data || !data->rigidBody || !data->is_valid) return;

        btTransform trans;
        data->rigidBody->getMotionState()->getWorldTransform(trans);
        data->position = trans.getOrigin();

        btVector3 linear_velocity = data->rigidBody->getLinearVelocity();
        btVector3 angular_velocity = data->rigidBody->getAngularVelocity();

        // BEM force calculation
        auto [thrust, torque] = calculate_bem_forces(data, linear_velocity);
        data->current_thrust = thrust;

        // Apply forces and torques
        btVector3 thrust_force = data->rotorNormal * thrust;
        data->rigidBody->applyCentralForce(thrust_force);

        btVector3 torque_vector = -data->rotorNormal * torque;
        data->rigidBody->applyTorque(torque_vector);

        // Power calculation
        const float omega = (data->currentRPM * 2.0f * glm::pi<float>()) / 60.0f;
        data->powerConsumption = std::abs(torque * omega);

        // Apply damping for stability
        data->rigidBody->setDamping(0.2f, 0.7f);
    }

    // Other methods remain the same...
    void drone_component::set_rpm(float target_rpm) {
        auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return;
        data->currentRPM = std::clamp(target_rpm, 0.0f, MAX_RPM);
    }

    float drone_component::get_thrust() const {
        const auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return 0.0f;
        return data->current_thrust;
    }

    float drone_component::get_power_consumption() const {
        const auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return 0.0f;
        return data->powerConsumption;
    }

    drone_component create(init_info info, drone_entity::entity entity) {
        return drone_component{ pool.create(info, entity) };
    }

    void remove(drone_component c) {
        pool.remove(c.get_id());
    }
};