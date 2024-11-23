#include "glad\glad.h"
#include "../Geometry/Geometry.h"
#include "../Geometry/GeometryRenderer.h"

class GeometryViewerView {
public:
    static GeometryViewerView& Get() {
        static GeometryViewerView instance;
        return instance;
    }

    void Draw();
    void HandleInput();
    void EnsureFramebuffer(float x, float y);
    void SetGeometry(std::shared_ptr<drosim::editor::Geometry> geometry);
    glm::mat4 CalculateViewMatrix();

private:
    // OpenGL rendering resources
    GLuint m_framebuffer = 0;
    GLuint m_colorTexture = 0;
    GLuint m_depthTexture = 0;
    
    // Basic camera controls
    float m_cameraPosition[3] = {0.0f, 0.0f, -5.0f};
    float m_cameraRotation[2] = {0.0f, 0.0f}; // pitch, yaw
    
    std::shared_ptr<drosim::editor::Geometry> m_currentGeometry;
};