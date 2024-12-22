#pragma once
#include "PhysicsBackend.h"

class GpuPhysicsBackend : public PhysicsBackend {
  public:
    GpuPhysicsBackend(RigidBodyArrays &rb, const size_t count)
      : rbData(rb), bodyCount(static_cast<GLuint>(count)), maxPairs(static_cast<GLuint>(count * 10)) {
      // Create/compile shader
      initComputeShaders();

      // create SSBOs for physics data
      createPhysicsSSBOs();

      // Create SSBOs for BVH
      createBVHSSBOs();

      // Create SSBOs for collision data
      createCollisionSSBOs();
    };

    ~GpuPhysicsBackend() override {
      // Delete physics SSBOs
      glDeleteBuffers(1, &positionBuffer);
      glDeleteBuffers(1, &orientationBuffer);
      glDeleteBuffers(1, &linearVelBuffer);
      glDeleteBuffers(1, &angularVelBuffer);
      glDeleteBuffers(1, &massBuffer);
      glDeleteBuffers(1, &inertiaBuffer);

      // Delete BVH SSBOs
      glDeleteBuffers(1, &mortonCodesBuffer);
      glDeleteBuffers(1, &sortedMortonCodesBuffer);
      glDeleteBuffers(1, &indicesBuffer);
      glDeleteBuffers(1, &sortedIndicesBuffer);
      glDeleteBuffers(1, &bvhNodesBuffer);

      // Delete collision SSBOs
      glDeleteBuffers(1, &collisionPairsBuffer);
      glDeleteBuffers(1, &collisionCountBuffer); // Add this line

      // Delete shader programs
      glDeleteProgram(physicsProgram);
      glDeleteProgram(mortonProgram);
      glDeleteProgram(sortProgram);
      glDeleteProgram(bvhProgram);
      glDeleteProgram(refitProgram);
      glDeleteProgram(collisionProgram);
    }

    void updateRigidBodies(size_t count, float dt) override;
    void detectCollisions(float dt) override;
    void resolveCollisions(float dt) override {};

  private:
    RigidBodyArrays &rbData;
    GLuint bodyCount;
    GLuint maxPairs;

    // Shader Programs
    GLuint physicsProgram = 0;
    GLuint mortonProgram = 0;
    GLuint sortProgram = 0;
    GLuint bvhProgram = 0;
    GLuint refitProgram = 0;
    GLuint collisionProgram = 0;

    GLint dtLocation = -1;
    GLint gravityLocation = -1;

    // Physics SSBOs
    GLuint positionBuffer = 0;
    GLuint orientationBuffer = 0;
    GLuint linearVelBuffer = 0;
    GLuint angularVelBuffer = 0;
    GLuint massBuffer = 0;
    GLuint inertiaBuffer = 0;

    // BVH SSBOs
    GLuint mortonCodesBuffer = 0;
    GLuint sortedMortonCodesBuffer = 0;
    GLuint indicesBuffer = 0;
    GLuint sortedIndicesBuffer = 0;
    GLuint bvhNodesBuffer = 0;

    // Collision SSBOs
    GLuint collisionPairsBuffer = 0;
    GLuint collisionCountBuffer = 0;

    // Scene Bounds for Morton Encoding
    glm::vec3 sceneMin = glm::vec3(-1000.0f);
    glm::vec3 sceneMax = glm::vec3(1000.0f);

    void initComputeShaders();
    void createPhysicsSSBOs();
    void createBVHSSBOs();
    void createCollisionSSBOs();
    static void createSSBO(GLuint &buffer, size_t size);

    void uploadPhysicsData();
    void bindPhysicsSSBOs();
    void downloadPhysicsData();

    void downloadCollisionData();

    GLuint createComputeProgram(const std::string &shaderName);

    // Helper Functions
    std::string loadFileAsString(const std::string& path) {
      FILE* f = fopen(path.c_str(), "rb");
      if (!f) return "";
      fseek(f, 0, SEEK_END);
      long size = ftell(f);
      fseek(f, 0, SEEK_SET);
      std::string data(size, '\0');
      fread(&data[0], 1, size, f);
      fclose(f);
      return data;
    }

    template<typename T>
    void uploadData(GLuint buffer, const std::vector<T> &data, bool padVec3ToVec4) {
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
      if (padVec3ToVec4 && sizeof(T) == sizeof(glm::vec3)) {
        std::vector<glm::vec4> temp(data.size());
        for (size_t i = 0; i < data.size(); ++i) {
          const glm::vec3 &v = reinterpret_cast<const glm::vec3&>(data[i]);
          temp[i] = glm::vec4(v, 0.0f);
        }
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, temp.size()*sizeof(glm::vec4), temp.data());
      } else if (padVec3ToVec4 && sizeof(T) == sizeof(glm::quat)) {
        // glm::quat is already vec4
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, data.size()*sizeof(glm::quat), data.data());
      } else {
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, data.size()*sizeof(T), data.data());
      }
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    // Overload for positions/orientations etc:
    void uploadData(GLuint buffer, const std::vector<glm::vec3> &data, bool pad) {
      // Specialized for vec3 -> vec4
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
      std::vector<glm::vec4> temp(data.size());
      for (size_t i = 0; i < data.size(); ++i) {
        temp[i] = glm::vec4(data[i], 0.0f);
      }
      glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, temp.size()*sizeof(glm::vec4), temp.data());
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void uploadData(GLuint buffer, const std::vector<glm::quat> &data, bool) {
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
      glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, data.size()*sizeof(glm::quat), data.data());
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void uploadData(GLuint buffer, const std::vector<glm::vec4> &data, bool) {
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
      glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, data.size()*sizeof(glm::vec4), data.data());
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    template<typename T>
    void downloadData(GLuint buffer, std::vector<T> &data, bool wasVec3Padded) {
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
      void* ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, data.size()*sizeof(T)* (wasVec3Padded? (sizeof(T)==sizeof(glm::vec3)?(4.0/3.0):1 ):1 ), GL_MAP_READ_BIT);
      if (ptr) {
        // If wasVec3Padded && T = glm::vec3, data on GPU is actually vec4
        // Need to map back from vec4 to vec3
        if (wasVec3Padded && sizeof(T)==sizeof(glm::vec3)) {
          std::vector<glm::vec4> temp(data.size());
          memcpy(temp.data(), ptr, temp.size()*sizeof(glm::vec4));
          for (size_t i = 0; i < data.size(); ++i) {
            data[i] = glm::vec3(temp[i]);
          }
        } else {
          memcpy(data.data(), ptr, data.size()*sizeof(T));
        }
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
      }
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void downloadData(GLuint buffer, std::vector<glm::quat> &data, bool) {
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
      void* ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, data.size()*sizeof(glm::quat), GL_MAP_READ_BIT);
      if (ptr) {
        memcpy(data.data(), ptr, data.size()*sizeof(glm::quat));
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
      }
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void downloadData(GLuint buffer, std::vector<glm::vec4> &data, bool) {
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
      void* ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, data.size()*sizeof(glm::vec4), GL_MAP_READ_BIT);
      if (ptr) {
        memcpy(data.data(), ptr, data.size()*sizeof(glm::vec4));
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
      }
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
};