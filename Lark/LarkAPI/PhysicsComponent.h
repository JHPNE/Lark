#pragma once
#include "../Components/ComponentCommon.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"

namespace lark::physics
{

DEFINE_TYPED_ID(physics_id);

class component final
{
  public:
    constexpr explicit component(physics_id id) : _id{id} {}
    constexpr component() : _id{id::invalid_id} {}
    constexpr physics_id get_id() const { return _id; }
    constexpr bool is_valid() const { return id::is_valid(_id); }

    [[nodiscard]] btRigidBody* get_rigid_body() const;

    void apply_force(const math::v3& force, const math::v3& position=math::v3(0.f));
    void apply_torque(const math::v3& torque);

    void get_state(math::v3& position, math::v4& orientation, math::v3& velocity, math::v3& angular_velocity) const;

  private:
    physics_id _id;
};
} // namespace lark::physics
