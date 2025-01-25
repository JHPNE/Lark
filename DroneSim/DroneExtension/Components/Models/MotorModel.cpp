// MotorModel.cpp
#include "MotorModel.h"

// In MotorModel.cpp
namespace lark::models {
    namespace {
        float calculate_back_emf(float rpm, float kv_rating) {
            const float rpm_to_radps = PI / 30.0f;
            float angular_velocity = rpm * rpm_to_radps;
            return angular_velocity / (kv_rating * RPM_TO_RAD);
        }

        float calculate_motor_current(
            float voltage,
            float back_emf,
            float resistance,
            float max_current,
            float winding_temperature) {

            // Temperature-dependent resistance
            float temp_coefficient = 0.004f; // Copper temperature coefficient
            float adjusted_resistance = resistance *
                (1.0f + temp_coefficient * (winding_temperature - 20.0f));

            float current = (voltage - back_emf) / adjusted_resistance;
            return std::clamp(current, -max_current, max_current);
        }

        float calculate_motor_torque(float current, float kv_rating, float efficiency_factor) {
            const float torque_constant = 60.0f / (2.0f * PI * kv_rating);
            return current * torque_constant * efficiency_factor;
        }

        float calculate_copper_losses(float current, float resistance, float temperature) {
            // Temperature-dependent copper losses
            float temp_coefficient = 0.004f;
            float adjusted_resistance = resistance *
                (1.0f + temp_coefficient * (temperature - 20.0f));
            return current * current * adjusted_resistance;
        }

        float calculate_iron_losses(float back_emf, float rpm) {
            // Enhanced iron loss model with frequency dependence
            float frequency = rpm / 60.0f;
            float hysteresis_loss = 0.05f * back_emf * back_emf;
            float eddy_current_loss = 0.03f * back_emf * back_emf * frequency;
            return hysteresis_loss + eddy_current_loss;
        }

        float calculate_mechanical_losses(float rpm, float load_torque) {
            // Enhanced mechanical loss model
            float friction_loss = 0.02f * std::abs(rpm);
            float windage_loss = 0.001f * std::pow(rpm, 2) / 1000000.0f;
            float bearing_loss = 0.1f * std::abs(load_torque * rpm / 1000.0f);
            return friction_loss + windage_loss + bearing_loss;
        }
    }

    MotorState calculate_motor_state(
        const MotorParameters& params,
        float demanded_rpm,
        float load_torque,
        float ambient_temperature,
        float delta_time) {

        MotorState state{};

        // Calculate back EMF
        state.back_emf = calculate_back_emf(demanded_rpm, params.kv_rating);

        // Calculate efficiency factor based on operating point
        float speed_factor = std::min(demanded_rpm / 10000.0f, 1.0f);
        float load_factor = std::min(std::abs(load_torque) /
            (params.voltage * params.max_current), 1.0f);
        float efficiency_factor = 0.95f * (1.0f - 0.2f * speed_factor * load_factor);

        // Calculate current with temperature compensation
        float current = calculate_motor_current(
            params.voltage,
            state.back_emf,
            params.resistance,
            params.max_current,
            state.winding_temperature
        );

        // Calculate torque with efficiency
        state.current_torque = calculate_motor_torque(current, params.kv_rating,
                                                    efficiency_factor);

        // Calculate losses
        float copper_losses = calculate_copper_losses(current, params.resistance,
                                                    state.winding_temperature);
        float iron_losses = calculate_iron_losses(state.back_emf, demanded_rpm);
        float mechanical_losses = calculate_mechanical_losses(demanded_rpm, load_torque);

        float total_losses = copper_losses + iron_losses + mechanical_losses;

        // Update power consumption and efficiency
        state.power_consumption = params.voltage * current;
        float output_power = state.current_torque * demanded_rpm * RPM_TO_RAD;

        if (state.power_consumption > 0) {
            state.efficiency = std::min(output_power / state.power_consumption, 0.95f);
        } else {
            state.efficiency = 0;
        }

        // Enhanced thermal model with improved cooling
        float heat_generation = total_losses;
        float cooling_factor = 1.0f + 0.5f * std::pow(demanded_rpm / 10000.0f, 0.7f);
        float effective_thermal_resistance = params.thermal_resistance / cooling_factor;

        float temperature_rise = (heat_generation * effective_thermal_resistance -
                               (state.winding_temperature - ambient_temperature)) /
                               params.thermal_capacity;

        state.winding_temperature += temperature_rise * delta_time;

        return state;
    }
}