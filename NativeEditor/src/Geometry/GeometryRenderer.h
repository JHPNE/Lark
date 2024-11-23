#include "glad\glad.h"
#include "Geometry.h"

class GeometryRenderer {
  public:
    struct VertexBuffer {
      GLuint vao = 0;
      GLuint vbo = 0;
      GLuint ibo = 0;
      size_t indexCount = 0;
    };

    static bool Initialize();
    static void Shutdown();

    // Geometry to VertexBuffer OpenGL
    static VertexBuffer CreateBuffersFromGeometry(const drosim::editor::Geometry* geometry);

    // Basic rendering with minimal shader
    static void RenderGeometry(const VertexBuffer& buffer,
                           const glm::mat4& projection,
                           const glm::mat4& view,
                           const glm::mat4& model);

  private:
    static GLuint m_basicShader;
};