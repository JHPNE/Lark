#pragma once
#include "../Components/ComponentCommon.h"
#include "PhysicExtension/Utils/DroneState.h"

namespace lark::drone
{

    DEFINE_TYPED_ID(drone_id);

    class component final
    {
    public:
        constexpr explicit component(drone_id id) : _id{id} {}
        constexpr component() : _id{id::invalid_id} {}
        constexpr drone_id  get_id() const { return _id; }
        constexpr bool is_valid() const { return id::is_valid(_id); }

        // Drone operations
        void update(float dt, const Eigen::Vector3f& wind);
        std::pair<Eigen::Vector3f, Eigen::Vector3f> get_forces_and_torques() const;

        DroneState get_state() const;
        void set_state(const DroneState& state);

        // Sync from physics
        void sync_from_physics(const math::v3& position, const math::v4& orientation,
                               const math::v3& velocity, const math::v3& angular_velocity);

    private:
        drone_id _id;
    };
} // namespace lark::physics
