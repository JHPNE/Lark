#include "MotorModel.h"

namespace lark::models {
    namespace {
        float calculate_back_emf(float rpm, float kv_rating) {
            const float rpm_to_radps = PI / 30.0f;
            return (rpm * rpm_to_radps) / (kv_rating * RPM_TO_RAD);
        }

        float calculate_motor_current(
            float voltage,
            float back_emf,
            float resistance,
            float max_current) {
            
            float current = (voltage - back_emf) / resistance;
            return std::clamp(current, -max_current, max_current);
        }

        float calculate_motor_torque(float current, float kv_rating) {
            const float torque_constant = 60.0f / (2.0f * PI * kv_rating);
            return current * torque_constant;
        }

        float calculate_copper_losses(float current, float resistance) {
            return current * current * resistance;
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

        // Calculate current draw
        float current = calculate_motor_current(
            params.voltage,
            state.back_emf,
            params.resistance,
            params.max_current
        );

        // Calculate motor torque
        state.current_torque = calculate_motor_torque(current, params.kv_rating);

        // Calculate losses and efficiency
        float copper_losses = calculate_copper_losses(current, params.resistance);
        float iron_losses = 0.1f * state.back_emf * state.back_emf; // Simplified iron loss model
        float mechanical_losses = 0.05f * std::abs(demanded_rpm); // Simple friction model

        state.power_consumption = params.voltage * current;
        float output_power = state.current_torque * demanded_rpm * RPM_TO_RAD;
        float total_losses = copper_losses + iron_losses + mechanical_losses;

        // Calculate efficiency
        if (state.power_consumption > 0) {
            state.efficiency = output_power / state.power_consumption;
        } else {
            state.efficiency = 0;
        }

        // Temperature calculation using thermal model
        float heat_generation = total_losses;
        float temperature_rise = (heat_generation * params.thermal_resistance - 
                               (state.winding_temperature - ambient_temperature)) / 
                               params.thermal_capacity;
        
        state.winding_temperature += temperature_rise * delta_time;

        return state;
    }
}