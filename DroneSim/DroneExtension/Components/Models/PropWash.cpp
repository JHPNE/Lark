#include "PropWash.h"

namespace lark::models {
    PropWashField calculate_prop_wash(btVector3 rotorNormal, const float rpm, const float area, float radius, int blade_count, const AtmosphericConditions& conditions, float thrust) {
          PropWashField prop_wash {};

          const float omega = rpm * RPM_TO_RAD;
          if (omega <= 0.0f) return prop_wash;

      const float induced_velocity = std::sqrt(thrust / (2.0f * conditions.density * area));

      constexpr float wake_expansion_rate = 0.15f;
      const float wake_radius = radius * (1.0f + wake_expansion_rate);

      const float circulation = thrust / (conditions.density * omega * radius * blade_count);
      const float tip_vortex_strength = circulation * 0.8f;

      prop_wash.velocity = rotorNormal * induced_velocity;
      prop_wash.vorticity = rotorNormal * (tip_vortex_strength / (2.0f * PI * wake_radius));
      prop_wash.intensity = thrust / (conditions.density * area * std::pow(induced_velocity, 2));

      return prop_wash;
    };

  float calculate_prop_wash_influence(const models::PropWashField& wash, const btVector3& wash_origin, const btVector3& affected_point, float rotor_radius) {
    btVector3 displacement = affected_point - wash_origin;
    float vertical_distance = displacement.dot(wash.velocity.normalized());

    if (vertical_distance < 0) return 0.0f;

    float radial_distance = (displacement - displacement.dot(wash.velocity.normalized()) * wash.velocity.normalized()).length();

    float wake_radius = rotor_radius * (1.0f + 0.15f * vertical_distance / rotor_radius);

    float radial_factor = std::exp(-std::pow(radial_distance / wake_radius, 2));

    float vertical_factor = std::exp(-vertical_distance / (3.0f * rotor_radius));
    return wash.intensity * radial_factor * vertical_factor;
  }
}
