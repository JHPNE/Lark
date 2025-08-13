#include "Wind.h"

namespace lark::physics::wind {
    DrydenGust::DrydenGust(const  Parameters& p)
        : params{p}
        , rng(std::random_device{}())
        , white_noise(0.0f, 1.0f) {

        V = glm::length(params.mean_wind);
        if (V < 0.1f) V = 10.0f;

        b = params.wingspan / 2.0f;

        computeTurbulenceParameters();
    }

    DrydenGust::~DrydenGust() = default;

    void DrydenGust::computeTurbulenceParameters() {
        // altitude in feet cause MIL-F-8785C formulas need it that way
        float h_ft = params.altitude * 3.2804f;

        // clamp see what we will do here in future
        h_ft = glm::clamp(h_ft, 10.0f, 10000.0f);

        if (h_ft <= 1000.0f) {
            // Low altitude
            L_w = h_ft / 3.28084f;
            L_u = h_ft / std::pow(0.177f + 0.000823f * h_ft, 0.4f) / 3.28084f;
            L_v = L_u;
        } else {
            // Medium/high altitude
            L_u = L_v = 1750.0f / 3.28084f;
            L_w = params.altitude;
        }

        float base_intensity = params.turbulence_level;

        if (h_ft <= 1000.0f) {
            // Low altitude turbulence
            sigma_w = 0.1f * base_intensity * (10.0f + h_ft / 100.0f);
        } else if (h_ft <= 2000.0f) {
            // Medium altitude
            sigma_w = base_intensity * 13.0f;
        } else {
            // High altitude
            sigma_w = base_intensity * 15.0f;
        }

        sigma_u = sigma_w;
        sigma_v = sigma_w;
    }

    float DrydenGust::filterFirstOrder(float input, float T, float K, FilterState& state, float dt) {
        // Implements H(s) = K / (1 + T*s)
        // Using bilinear transform: s = (2/dt) * (z-1)/(z+1)

        float alpha = dt / (2.0f * T);
        float a0 = 1.0f + alpha;
        float a1 = -(1.0f - alpha) / a0;
        float b0 = K * alpha / a0;
        float b1 = b0;

        float output = b0 * input + b1 * input + a1 * state.y1;
        state.y1 = output;

        return output;
    }

    float DrydenGust::filterSecondOrder(float input, float T, float K, FilterState& state, float dt) {
        // Implements H(s) = K * (1 + sqrt(3)*T*s) / (1 + T*s)^2
        // This matches the Wikipedia transfer functions for v_g and w_g

        const float sqrt3 = std::sqrt(3.0f);

        // Discrete implementation using state-space or direct form
        float alpha = dt / (2.0f * T);

        // Numerator: K * (1 + sqrt(3)*T*s)
        float b0 = K * (1.0f + sqrt3 * alpha) / ((1.0f + alpha) * (1.0f + alpha));
        float b1 = K * (1.0f - sqrt3 * alpha) / ((1.0f + alpha) * (1.0f + alpha));

        // Denominator: (1 + T*s)^2
        float a0 = 1.0f;
        float a1 = -2.0f * (1.0f - alpha) / (1.0f + alpha);
        float a2 = ((1.0f - alpha) * (1.0f - alpha)) / ((1.0f + alpha) * (1.0f + alpha));

        float output = b0 * input + b1 * state.u1_2nd - a1 * state.y1_2nd - a2 * state.y2_2nd;

        // Update states
        state.y2_2nd = state.y1_2nd;
        state.y1_2nd = output;
        state.u1_2nd = input;

        return output;
    }

    glm::vec3 DrydenGust::update(float time, const glm::vec3& position) {
        float dt = time - last_time;
        if (dt <= 0.0f) dt = 0.01f;  // Default timestep
        last_time = time;

        // Generate white noise inputs
        float n_u = white_noise(rng);
        float n_v = white_noise(rng);
        float n_w = white_noise(rng);

        // Compute filter gains K from transfer functions
        // Per Wikipedia: G_ug(s) = sigma_u * sqrt(2*L_u/(pi*V)) * 1/(1 + L_u*s/V)
        float K_u = sigma_u * std::sqrt(2.0f * L_u / (M_PI * V));
        float T_u = L_u / V;

        // Per Wikipedia: G_vg(s) = sigma_v * sqrt(2*L_v/(pi*V)) * (1 + sqrt(3)*L_v*s/V)/(1 + 2*L_v*s/V)^2
        float K_v = sigma_v * std::sqrt(2.0f * L_v / (M_PI * V));
        float T_v = 2.0f * L_v / V;  // Note the factor of 2 in denominator

        // Same for w
        float K_w = sigma_w * std::sqrt(2.0f * L_w / (M_PI * V));
        float T_w = 2.0f * L_w / V;

        // Apply filters
        float gust_u = filterFirstOrder(n_u, T_u, K_u, filter_u, dt);
        float gust_v = filterSecondOrder(n_v, T_v, K_v, filter_v, dt);
        float gust_w = filterSecondOrder(n_w, T_w, K_w, filter_w, dt);

        // Return total wind (mean + turbulence)
        // Note: gusts are in body frame, may need rotation to world frame
        return params.mean_wind + glm::vec3(gust_u, gust_v, gust_w);
    }
}