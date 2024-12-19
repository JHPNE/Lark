#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>

struct Environment {
  glm::vec3 Gravity = glm::vec3(0.0f, -9.81f, 0.0f);
};

struct RigidBodyArrays {
  std::vector<glm::vec3> positions;
  std::vector<glm::quat> orientations;
  std::vector<glm::vec3> linearVelocities;
  std::vector<glm::vec3> angularVelocities;

  // For mass/inertia:
  // mass and invMass are scalars
  // Inertia and invInertia are diagonal vectors (ix, iy, iz)
  // We'll store them in two arrays of vec4 because the shader expects vec4:
  // Layout:
  // massData[i]   = (mass, invMass, inertia.x, inertia.y)
  // inertiaData[i]= (inertia.z, invInertia.x, invInertia.y, invInertia.z)
  std::vector<glm::vec4> massData;
  std::vector<glm::vec4> inertiaData;
};