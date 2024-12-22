#include "GpuPhysicsBackend.h"

#include <algorithm>

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

  dtLocation = glGetUniformLocation(physicsProgram, "dt");
  gravityLocation = glGetUniformLocation(physicsProgram, "gravity");
  if (dtLocation == -1) {
    std::cerr << "Uniform 'dt' not found in physicsProgram.\n";
  }
  if (gravityLocation == -1) {
    std::cerr << "Uniform 'Gravity' not found in physicsProgram.\n";
  }
  //TODO Cache rest
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
  // Define maximum number of collision pairs
  maxPairs = bodyCount * 10; // Assuming 10 collision pairs per body

  // Create SSBO for collision pairs using glm::uvec2
  createSSBO(collisionPairsBuffer, maxPairs * sizeof(glm::uvec2));

  // Create Atomic Counter Buffer for Collision Count
  glGenBuffers(1, &collisionCountBuffer);
  glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, collisionCountBuffer);
  glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
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

void GpuPhysicsBackend::detectCollisions(float dt) {
  if (!mortonProgram || !sortProgram || !bvhProgram || !collisionProgram) return;

  // compute morton codes
  glUseProgram(mortonProgram);
  glUniform3f(glGetUniformLocation(mortonProgram, "sceneMin"), sceneMin.x, sceneMin.y, sceneMin.z);
  glUniform3f(glGetUniformLocation(mortonProgram, "sceneMax"), sceneMax.x, sceneMax.y, sceneMax.z);
  GLint location = glGetUniformLocation(mortonProgram, "NUM_OBJECTS");
  if (location != -1) {
    glUniform1ui(location, bodyCount);
  } else {
    std::cerr << "Uniform 'NUM_OBJECTS' not found in mortonProgram.\n";
  };

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, positionBuffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mortonCodesBuffer);
  // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, boundsBuffer); // If using bounds

  GLuint mortonGroups = (GLuint)((bodyCount + 256 - 1) / 256);
  glDispatchCompute(mortonGroups, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  // radix sort
  glUseProgram(sortProgram);
  GLint sortNumObjectsLoc = glGetUniformLocation(sortProgram, "NUM_OBJECTS");
  if (sortNumObjectsLoc != -1) {
    glUniform1ui(sortNumObjectsLoc, bodyCount);
  } else {
    std::cerr << "Uniform 'NUM_OBJECTS' not found in sortProgram.\n";
  }

  for (int pass = 0; pass < 4; pass++) {
    glUniform1i(glGetUniformLocation(sortProgram, "bitOffset"), pass * 8);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mortonCodesBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sortedMortonCodesBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, indicesBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, sortedIndicesBuffer);

    GLuint sortGroups = (GLuint)((bodyCount + 255) / 256);
    glDispatchCompute(sortGroups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Swap buffers for next pass
    std::swap(mortonCodesBuffer, sortedMortonCodesBuffer);
    std::swap(indicesBuffer, sortedIndicesBuffer);
  }

  // build lvbh
  glUseProgram(bvhProgram);
  if (const GLint bvhNumObjectsLoc = glGetUniformLocation(bvhProgram, "NUM_OBJECTS") != -1) {
    glUniform1ui(bvhNumObjectsLoc, bodyCount);
  } else {
    std::cerr << "Uniform 'NUM_OBJECTS' not found in bvhProgram.\n";
  }

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sortedMortonCodesBuffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sortedIndicesBuffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, positionBuffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, bvhNodesBuffer);

  GLuint bvhGroups = (GLuint)((bodyCount + 256 - 1) / 256);
  glDispatchCompute(bvhGroups, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  // Step 4: Refit BVH
  glUseProgram(refitProgram);
  GLint refitNumObjectsLoc = glGetUniformLocation(refitProgram, "NUM_OBJECTS");
  if (refitNumObjectsLoc != -1) {
    glUniform1ui(refitNumObjectsLoc, bodyCount);
  } else {
    std::cerr << "Uniform 'NUM_OBJECTS' not found in refitProgram.\n";
  }

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, bvhNodesBuffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, positionBuffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, orientationBuffer);

  GLuint refitGroups = (GLuint)((bodyCount + 256 - 1) / 256);
  glDispatchCompute(refitGroups, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  // Step 5: Collision Detection
  glUseProgram(collisionProgram);
  GLint collisionNumObjectsLoc = glGetUniformLocation(collisionProgram, "NUM_OBJECTS");
  if (collisionNumObjectsLoc != -1) {
    glUniform1ui(collisionNumObjectsLoc, bodyCount);
  } else {
    std::cerr << "Uniform 'NUM_OBJECTS' not found in collisionProgram.\n";
  }
  glUniform1f(glGetUniformLocation(collisionProgram, "dt"), dt);

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, bvhNodesBuffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sortedIndicesBuffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, collisionPairsBuffer);
  glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 6, collisionCountBuffer);

  // Initialize atomic counter to zero
  GLuint zero = 0;
  glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, collisionCountBuffer);
  glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &zero);
  glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

  GLuint collisionGroups = (GLuint)((bodyCount + 256 - 1) / 256);
  glDispatchCompute(collisionGroups, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

  // Download collision data
  downloadCollisionData();
}

void GpuPhysicsBackend::downloadCollisionData() {
  // Read the atomic counter value
  GLuint count;
  glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, collisionCountBuffer);
  void* ptr = glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT);
  if (ptr) {
    count = *((GLuint*)ptr);
    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
  }
  glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

  // Limit count to maxPairs to prevent overflow
  count = glm::min(count, static_cast<GLuint>(maxPairs));

  // Download collision pairs
  std::vector<glm::uvec2> collisionPairs(count);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, collisionPairsBuffer);
  void* dataPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, count * sizeof(glm::uvec2), GL_MAP_READ_BIT);
  if (dataPtr) {
    memcpy(collisionPairs.data(), dataPtr, count * sizeof(glm::uvec2));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
  }
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  // Process collision pairs as needed
  for (const auto& pair : collisionPairs) {
    size_t indexA = pair.x;
    size_t indexB = pair.y;

    // Ensure indices are within bounds
    if (indexA >= rbData.linearVelocities.size() || indexB >= rbData.linearVelocities.size()) {
      continue;
    }

    // Simple collision response: swap velocities (elastic collision)
    glm::vec3 velA = glm::vec3(rbData.linearVelocities[indexA].x, rbData.linearVelocities[indexA].y, rbData.linearVelocities[indexA].z);
    glm::vec3 velB = glm::vec3(rbData.linearVelocities[indexB].x, rbData.linearVelocities[indexB].y, rbData.linearVelocities[indexB].z);

    rbData.linearVelocities[indexA].x = velB.x;
    rbData.linearVelocities[indexA].y = velB.y;
    rbData.linearVelocities[indexA].z = velB.z;

    rbData.linearVelocities[indexB].x = velA.x;
    rbData.linearVelocities[indexB].y = velA.y;
    rbData.linearVelocities[indexB].z = velA.z;

    // Additional collision response logic can be implemented here
  }

  // After resolving, upload updated velocities back to GPU
  uploadPhysicsData();
}
