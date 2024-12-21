#include "GpuPhysicsBackend.h"

void GpuPhysicsBackend::initComputeShaders() {
  // Load and compile all compute shaders
  physicsProgram = createComputeProgram(PhysicsBackend::GetCompShader(drosim::physics::shaders::physics_update));
  mortonProgram = createComputeProgram(PhysicsBackend::GetCompShader(drosim::physics::shaders::morton_codes));
  sortProgram = createComputeProgram(PhysicsBackend::GetCompShader(drosim::physics::shaders::radix_sort));
  bvhProgram = createComputeProgram(PhysicsBackend::GetCompShader(drosim::physics::shaders::build_lbvh));
  refitProgram = createComputeProgram(PhysicsBackend::GetCompShader(drosim::physics::shaders::refit_bvh));
  collisionProgram = createComputeProgram(PhysicsBackend::GetCompShader(
      drosim::physics::shaders::collision_detection));

  if (!physicsProgram || !mortonProgram || !sortProgram || !bvhProgram || !refitProgram || !collisionProgram) {
    std::cerr << "Failed to initialize one or more compute shaders.\n";
  }
};

void GpuPhysicsBackend::createSSBO(GLuint &buffer, size_t size) {
  glGenBuffers(1, &buffer);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
  glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void GpuPhysicsBackend::createPhysicsSSBOs() {
  // Create SSBOs
  createSSBO(positionBuffer, rbData.positions.size() * sizeof(glm::vec4)); // We'll store pos as vec4
  createSSBO(orientationBuffer, rbData.orientations.size() * sizeof(glm::vec4));
  createSSBO(linearVelBuffer, rbData.linearVelocities.size() * sizeof(glm::vec4));
  createSSBO(angularVelBuffer, rbData.angularVelocities.size() * sizeof(glm::vec4));
  createSSBO(massBuffer, rbData.massData.size() * sizeof(glm::vec4));
  createSSBO(inertiaBuffer, rbData.inertiaData.size() * sizeof(glm::vec4));
}

void GpuPhysicsBackend::createBVHSSBOs() {
  // Create SSBOs for BVH construction
  createSSBO(mortonCodesBuffer, bodyCount * sizeof(uint32_t));
  createSSBO(sortedMortonCodesBuffer, bodyCount * sizeof(uint32_t));
  createSSBO(indicesBuffer, bodyCount * sizeof(uint32_t));
  createSSBO(sortedIndicesBuffer, bodyCount * sizeof(uint32_t));

  // BVH Nodes: assuming a binary tree, maximum nodes = 2 * bodyCount
  struct BVHNode {
    glm::vec4 boundsMin;
    glm::vec4 boundsMax;
    uint32_t leftChild;
    uint32_t rightChild;
    uint32_t parent;
    uint32_t isLeaf;
  };
  createSSBO(bvhNodesBuffer, 2 * bodyCount * sizeof(BVHNode));
}

void GpuPhysicsBackend::createCollisionSSBOs() {
  // Create SSBOs for storing collision pairs
  // Assuming a maximum number of collision pairs; adjust as needed
  size_t maxPairs = bodyCount * 10;
  createSSBO(collisionPairsBuffer, maxPairs * sizeof(glm::uvec2)); // Using uint2 for pairs
}

void GpuPhysicsBackend::uploadPhysicsData() {
  // Upload data
  uploadData(positionBuffer, rbData.positions, true);
  uploadData(orientationBuffer, rbData.orientations, true);
  uploadData(linearVelBuffer, rbData.linearVelocities, true);
  uploadData(angularVelBuffer, rbData.angularVelocities, true);
  uploadData(massBuffer, rbData.massData, false);
  uploadData(inertiaBuffer, rbData.inertiaData, false);
}

void GpuPhysicsBackend::bindPhysicsSSBOs() {
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, positionBuffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, orientationBuffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, linearVelBuffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, angularVelBuffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, massBuffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, inertiaBuffer);
}

void GpuPhysicsBackend::downloadPhysicsData() {
  // Download data
  downloadData(positionBuffer, rbData.positions, true);
  downloadData(orientationBuffer, rbData.orientations, true);
  downloadData(linearVelBuffer, rbData.linearVelocities, true);
  downloadData(angularVelBuffer, rbData.angularVelocities, true);
  downloadData(massBuffer, rbData.massData, false);
  downloadData(inertiaBuffer, rbData.inertiaData, false);
}

GLuint GpuPhysicsBackend::createComputeProgram(const std::string &shaderName) {
  std::string compSource = loadFileAsString(shaderName);
  if (compSource.empty()) {
    std::cerr << "Compute shader file not found. \n";
    return 0;
  };

  const char* src = compSource.c_str();
  GLuint comp = glCreateShader(GL_COMPUTE_SHADER);
  glShaderSource(comp, 1, &src, nullptr);
  glCompileShader(comp);

  GLint status = 0;
  glGetShaderiv(comp, GL_COMPILE_STATUS, &status);
  if (!status) {
    char log[512];
    glGetShaderInfoLog(comp, 512, nullptr, log);
    std::cerr << "Compute shader compilation failed: " << log << "\n";
    glDeleteShader(comp);
    return 0;
  }

  GLuint prog = glCreateProgram();
  glAttachShader(prog, comp);
  glLinkProgram(prog);

  glGetProgramiv(prog, GL_LINK_STATUS, &status);
  if (!status) {
    char log[512];
    glGetProgramInfoLog(prog, 512, nullptr, log);
    std::cerr << "Link shader program failed: " << log << "\n";
    glDeleteProgram(prog);
    glDeleteShader(comp);
    return 0;
  }

  glDeleteShader(comp);
  return prog;
}

void GpuPhysicsBackend::updateRigidBodies(size_t count, float dt) {
  if (!physicsProgram || count == 0) return;

  // Upload physics data
  uploadPhysicsData();

  glUseProgram(physicsProgram);
  glUniform1f(dtLocation, dt);
  glUniform3f(gravityLocation, GetEnvironment().Gravity.x, GetEnvironment().Gravity.y, GetEnvironment().Gravity.z);

  // Bind physics SSBOs
  bindPhysicsSSBOs();

  // Dispatch
  GLuint groups = (GLuint)((count + 63) / 64); // match local_size_x=64 in shader
  glDispatchCompute(groups, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  downloadPhysicsData();
}