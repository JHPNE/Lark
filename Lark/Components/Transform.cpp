#include "Transform.h"
#include "Entity.h"

namespace lark::transform {

namespace {
util::vector<math::v3> positions;
util::vector<math::v4> rotations; // Using vec4 for quaternions
util::vector<math::v3> scales;

math::v4 euler_to_quaternion(const math::v3& euler_angles) {
  // Convert euler angles from degrees to radians
  math::v3 radians = glm::radians(euler_angles);

  // Create quaternion from radians
  glm::quat q = glm::quat(radians);

  // Convert to math::v4 (x, y, z, w)
  return math::v4(q.x, q.y, q.z, q.w);
}

// TODO remove if there is no further use
math::v3 quaternion_to_euler(const math::v4 &quaternion) {
  glm::quat q(quaternion.w, quaternion.x, quaternion.y, quaternion.z);
  return glm::degrees(glm::eulerAngles(q));
}
} // namespace

void component::set_rotation(const math::v4 &rotation) {
  assert(is_valid());
  rotations[id::index(_id)] = glm::normalize(rotation);
}

void component::set_rotation_euler(const math::v3 &euler_angles) {
  set_rotation(euler_to_quaternion(euler_angles));
}

void component::set_scale(const math::v3 &new_scale) {
  assert(is_valid());
  // Prevent zero or negative scale
  scales[id::index(_id)] = glm::max(new_scale, math::v3(0.001f));
}

void component::set_position(const math::v3 &new_position) {
  assert(is_valid());
  positions[id::index(_id)] = new_position;
}

void component::translate(const math::v3 &translation) {
  assert(is_valid());
  positions[id::index(_id)] += translation;
}

void component::rotate(const math::v3 &euler_angles) {
  math::v4 current_quat = rotation();
  math::v4 rotation_quat = euler_to_quaternion(euler_angles);
  set_rotation(rotation_quat * current_quat);
}

void component::scale_by(const math::v3 &scale_factor) {
  assert(is_valid());
  scales[id::index(_id)] *= scale_factor;
  // Ensure scale doesn't go below minimum
  scales[id::index(_id)] = glm::max(scales[id::index(_id)], math::v3(0.001f));
}

math::m4x4 component::get_transform_matrix() const {
  assert(is_valid());
  const id::id_type index = id::index(_id);

  math::m4x4 transform = glm::mat4(1.0f);

  // Apply translation
  transform = glm::translate(transform, positions[index]);

  // Apply rotation (quaternion)
  glm::quat rotation_quat(rotations[index].w, rotations[index].x,
                          rotations[index].y, rotations[index].z);
  transform *= glm::mat4_cast(rotation_quat);

  // Apply scale
  transform = glm::scale(transform, scales[index]);

  return transform;
}

void component::reset() {
  assert(is_valid());
  const id::id_type index = id::index(_id);
  positions[index] = math::v3(0.0f);
  rotations[index] = math::v4(0.0f, 0.0f, 0.0f, 1.0f); // Identity quaternion
  scales[index] = math::v3(1.0f);
}

component create(init_info info, game_entity::entity entity) {
  assert(entity.is_valid());
  const id::id_type entity_index{id::index(entity.get_id())};

  if (positions.size() > entity_index) {
    // Convert arrays to GLM vectors
    rotations[entity_index] = math::v4(info.rotation[0], info.rotation[1],
                                       info.rotation[2], info.rotation[3]);
    positions[entity_index] =
        math::v3(info.position[0], info.position[1], info.position[2]);
    scales[entity_index] =
        math::v3(info.scale[0], info.scale[1], info.scale[2]);
  } else {
    assert(positions.size() == entity_index);
    rotations.emplace_back(math::v4(info.rotation[0], info.rotation[1],
                                    info.rotation[2], info.rotation[3]));
    positions.emplace_back(
        math::v3(info.position[0], info.position[1], info.position[2]));
    scales.emplace_back(math::v3(info.scale[0], info.scale[1], info.scale[2]));
  }
  return component(transform_id{(id::id_type)positions.size() - 1});
}

void remove(component t) { assert(t.is_valid()); }

math::v4 component::rotation() const {
  assert(is_valid());
  return rotations[id::index(_id)];
}

math::v3 component::scale() const {
  assert(is_valid());
  return scales[id::index(_id)];
}

math::v3 component::position() const {
  assert(is_valid());
  return positions[id::index(_id)];
}
} // namespace lark::transform