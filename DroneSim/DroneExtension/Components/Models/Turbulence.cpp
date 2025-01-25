#include "Turbulence.h"

namespace lark::models {
namespace {
    // Atmospheric condition parameters
    struct AtmosphericParams {
        float stability_parameter;  // Richardson number approximation
        float shear_factor;         // Wind shear intensity
        float terrain_roughness;    // Surface roughness length
    };

    float blend(float a, float b, float t) {
        float s = std::clamp(t, 0.0f, 1.0f);
        return a * (1.0f - s) + b * s;
    }

    AtmosphericParams calculate_atmospheric_params(float altitude, const AtmosphericConditions& conditions) {
        AtmosphericParams params;

        float temp_gradient = -0.0065f;  // Standard lapse rate (K/m)
        float actual_gradient = (conditions.temperature - ISA_SEA_LEVEL_TEMPERATURE) /
                                std::max(altitude, 1.0f);
        params.stability_parameter = (actual_gradient - temp_gradient) / temp_gradient;

        // Wind shear approximated by log law scaling
        params.shear_factor = 0.4f / std::log(std::max(altitude, 1.0f) / 0.1f);

        // Roughness length for typical terrain
        params.terrain_roughness = 0.1f;  // Moderate roughness (m)

        return params;
    }

    struct VonKarmanScales {
        float Lu;  // Longitudinal scale
        float Lv;  // Lateral scale
        float Lw;  // Vertical scale
    };

    VonKarmanScales calculate_von_karman_scales(float altitude) {
        VonKarmanScales scales;

        const float surface_Lu = 100.0f;
        const float free_Lu = 1000.0f;

        float blend_factor = std::clamp((altitude - 800.0f) / 400.0f, 0.0f, 1.0f);  // Smooth transition
        scales.Lu = blend(surface_Lu, free_Lu, blend_factor);
        scales.Lv = scales.Lu * 0.8f;  // Cross-stream scale
        scales.Lw = scales.Lu * 0.5f;  // Vertical scale

        return scales;
    }


    float pseudo_random(float seed) {
        return std::fmod(std::sin(seed * 43758.5453123f), 1.0f) - 0.5f;  // Range: [-0.5, 0.5]
    }

    btVector3 generate_turbulence(const VonKarmanScales& scales, const AtmosphericParams& params, float sigma, float time) {
        btVector3 turbulence;

        auto generate_component = [&](float scale, float intensity, float time_shift) {
            float omega = 2.0f * PI / scale;  // Frequency approximation
            float random_variation = 0.5f * pseudo_random(time + time_shift);  // Deterministic random variation
            float phase = omega * time + time_shift + random_variation;
            return intensity * std::sin(phase);
        };

        turbulence.setX(generate_component(scales.Lu, sigma * (1.0f + params.shear_factor), time));
        turbulence.setY(generate_component(scales.Lv, sigma, time + PI / 3.0f));
        turbulence.setZ(generate_component(scales.Lw, sigma * (1.0f - params.shear_factor), time - PI / 3.0f));

        return turbulence;
    }



    float calculate_sigma(float altitude, const AtmosphericParams& params) {
        const float surface_sigma = 0.15f;  // Surface layer intensity
        const float free_sigma = 0.1f;      // Free atmosphere intensity

        float blend_factor = std::clamp((altitude - 800.0f) / 400.0f, 0.0f, 1.0f);  // Transition around 1000m
        float sigma = blend(surface_sigma, free_sigma, blend_factor);

        return sigma * (1.0f + 0.1f * params.stability_parameter);
    }

    btVector3 calculate_angular_velocity(const btVector3& linear_velocity, const VonKarmanScales& scales) {
        return btVector3(
            linear_velocity.y() / scales.Lu,
            linear_velocity.z() / scales.Lv,
            linear_velocity.x() / scales.Lw
        ) * 0.2f;  // Scale to realistic magnitudes
    }

}  // namespace

TurbulenceState calculate_turbulence(float altitude, float airspeed, const AtmosphericConditions& conditions, float time) {
    AtmosphericParams params = calculate_atmospheric_params(altitude, conditions);
    VonKarmanScales scales = calculate_von_karman_scales(altitude);
    float sigma = calculate_sigma(altitude, params);

    btVector3 velocity = generate_turbulence(scales, params, sigma, time);
    btVector3 angular_velocity = calculate_angular_velocity(velocity, scales);

    TurbulenceState state;
    state.velocity = velocity;
    state.angular_velocity = angular_velocity;
    state.intensity = sigma;
    state.length_scale = scales.Lu;  // Set length_scale to the longitudinal scale
    state.time_scale = 1.0f / (2.0f * PI / scales.Lu);  // Optional: calculate time scale based on length scale

    return state;
}
}  // namespace lark::models
