#pragma once
#include "PhysicsBackend.h"

class GpuPhysicsBackend : public PhysicsBackend {
  public:
    GpuPhysicsBackend(RigidBodyArrays &rb, size_t count) : rbData(rb), bodyCount(count) {
      // Create/compile shader
      program = createComputeProgram(GetCompShader(rigid_body));
      if (!program) {
        std::cerr << "Failed to create compute shader program.\n";
      }

      // Get uniforms
      dtLocation = glGetUniformLocation(program, "dt");
      gravityLocation = glGetUniformLocation(program, "gravity");

      // Create SSBOs
      createSSBO(positionBuffer, rbData.positions.size() * sizeof(glm::vec4)); // We'll store pos as vec4
      createSSBO(orientationBuffer, rbData.orientations.size() * sizeof(glm::vec4));
      createSSBO(linearVelBuffer, rbData.linearVelocities.size() * sizeof(glm::vec4));
      createSSBO(angularVelBuffer, rbData.angularVelocities.size() * sizeof(glm::vec4));
      createSSBO(massBuffer, rbData.massData.size() * sizeof(glm::vec4));
      createSSBO(inertiaBuffer, rbData.inertiaData.size() * sizeof(glm::vec4));
    };

    ~GpuPhysicsBackend() {
      glDeleteBuffers(1, &positionBuffer);
      glDeleteBuffers(1, &orientationBuffer);
      glDeleteBuffers(1, &linearVelBuffer);
      glDeleteBuffers(1, &angularVelBuffer);
      glDeleteBuffers(1, &massBuffer);
      glDeleteBuffers(1, &inertiaBuffer);
      if (program) glDeleteProgram(program);
    }

    void updateRigidBodies(size_t count, float dt) override {
      if (!program || count == 0) return;

      // Upload data
      uploadData(positionBuffer, rbData.positions, true);
      uploadData(orientationBuffer, rbData.orientations, true);
      uploadData(linearVelBuffer, rbData.linearVelocities, true);
      uploadData(angularVelBuffer, rbData.angularVelocities, true);
      uploadData(massBuffer, rbData.massData, false);
      uploadData(inertiaBuffer, rbData.inertiaData, false);

      glUseProgram(program);
      glUniform1f(dtLocation, dt);
      glUniform3f(gravityLocation, GetEnvironment().Gravity.x, GetEnvironment().Gravity.y, GetEnvironment().Gravity.z);

      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, positionBuffer);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, orientationBuffer);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, linearVelBuffer);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, angularVelBuffer);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, massBuffer);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, inertiaBuffer);

      // Dispatch
      GLuint groups = (GLuint)((count + 63) / 64); // match local_size_x=64 in shader
      glDispatchCompute(groups, 1, 1);
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

      // Download data
      downloadData(positionBuffer, rbData.positions, true);
      downloadData(orientationBuffer, rbData.orientations, true);
      downloadData(linearVelBuffer, rbData.linearVelocities, true);
      downloadData(angularVelBuffer, rbData.angularVelocities, true);
      downloadData(massBuffer, rbData.massData, false);
      downloadData(inertiaBuffer, rbData.inertiaData, false);
    }

  private:
    RigidBodyArrays &rbData;
    size_t bodyCount;

    GLuint program = 0;
    GLint dtLocation = -1;
    GLint gravityLocation = -1;

    GLuint positionBuffer = 0;
    GLuint orientationBuffer = 0;
    GLuint linearVelBuffer = 0;
    GLuint angularVelBuffer = 0;
    GLuint massBuffer = 0;
    GLuint inertiaBuffer = 0;

    static void createSSBO(GLuint &buffer, size_t size) {
      glGenBuffers(1, &buffer);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
      glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    GLuint createComputeProgram(const std::string &shaderName) {
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