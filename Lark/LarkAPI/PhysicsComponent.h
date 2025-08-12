#pragma once
#include "../Components/ComponentCommon.h"
#include "Physics/DroneTypes.h"

namespace lark::physics {

    DEFINE_TYPED_ID(physics_id);

    class component final {
    public:
        constexpr explicit component(physics_id id) : _id{id} {}
        constexpr component() : _id{id::invalid_id} {}
        constexpr physics_id get_id() const { return _id; }
        constexpr bool is_valid() const { return id::is_valid(_id); }

        // Physics operations
        void step(float dt);
        void set_control_input(const drones::ControlInput& input);
        drones::DroneState get_state() const;
        void apply_wind(const glm::vec3& wind);

    private:
        physics_id _id;
    };
}
