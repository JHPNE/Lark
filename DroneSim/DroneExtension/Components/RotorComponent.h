#pragma once
#include "BaseComponent.h"

namespace lark::rotor {
    DEFINE_TYPED_ID(rotor_id);

    class drone_component final : public base_component<rotor_id> {
        public:
            using base_component<rotor_id>::base_component;

            void calculate_forces(float deltaTime);
            void set_rpm(float target_rpm);
            [[nodiscard]] float get_thrust() const;
            [[nodiscard]] float get_power_consumption() const;
    };
};