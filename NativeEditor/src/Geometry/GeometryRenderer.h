#include "glad/glad.h"
#include "Geometry.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class GeometryRenderer {
  public:
    struct MeshBuffers {
      GLuint vao = 0;
      GLuint vbo = 0;
      GLuint ibo = 0;
      GLsizei indexCount = 0;
      GLenum indexType = GL_UNSIGNED_INT;

      ~MeshBuffers() {
        if (vao) glDeleteVertexArrays(1, &vao);
        if (vbo) glDeleteBuffers(1, &vbo);
        if (ibo) glDeleteBuffers(1, &ibo);
      }
    };

    // Represents a LOD level containing multiple meshes
    struct LODLevelBuffers {
        std::string name;
        float threshold;
        std::vector<std::shared_ptr<MeshBuffers>> meshBuffers;
    };

    // Represents a complete LOD group
    struct LODGroupBuffers {
        std::string name;
        std::vector<std::shared_ptr<LODLevelBuffers>> lodLevels;
    };

    static bool Initialize();
    static void Shutdown();

    // Geometry to VertexBuffer OpenGL
    static std::unique_ptr<LODGroupBuffers> CreateBuffersFromGeometry(drosim::editor::Geometry* geometry);

  static void RenderGeometryAtLOD(const LODGroupBuffers* groupBuffers,
                             const glm::mat4& view,
                             const glm::mat4& projection,
                             float distanceToCamera);

  private:
    static GLuint m_basicShader;
    static std::shared_ptr<MeshBuffers> CreateMeshBuffers(const std::shared_ptr<drosim::editor::Mesh>& mesh);
    // Add shader compilation helper
    static GLuint CompileShader(GLenum type, const char* source);
    static void RenderMesh(const MeshBuffers* meshBuffers);


    static inline const char* s_basicVertexShader = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoord;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 Normal;
        out vec3 FragPos;

        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

  static inline const char* s_basicFragmentShader = R"(
        #version 330 core
        out vec4 FragColor;

        in vec3 Normal;
        in vec3 FragPos;

        uniform vec3 objectColor;

        void main() {
            FragColor = vec4(objectColor, 1.0);
        }
    )";
};
