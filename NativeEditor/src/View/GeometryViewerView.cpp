#include "GeometryViewerView.h"

void GeometryViewerView::SetUpViewport() {
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    
    // Create or resize framebuffer if needed
    EnsureFramebuffer(viewportSize.x, viewportSize.y);
    
    // Bind framebuffer and set viewport
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glViewport(0, 0, (GLsizei)viewportSize.x, (GLsizei)viewportSize.y);
    
    // Clear buffers
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
}

void GeometryViewerView::Draw() {
    if (!m_initialized) return;

    ImGui::Begin("Geometry Viewer");
    
    // Get the size of the viewport
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    
    // Set up viewport and framebuffer
    SetUpViewport();
    
    // Calculate view and projection matrices
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, m_cameraDistance),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    float aspectRatio = viewportSize.x / viewportSize.y;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
    
    // Render geometry
    if (m_geometryBuffers) {
        GeometryRenderer::RenderGeometryAtLOD(m_geometryBuffers.get(), view, projection, m_cameraDistance);
    }
    
     // Display the rendered texture in ImGui with correct casting
    ImGui::Image(reinterpret_cast<ImTextureID>((void*)(uintptr_t)m_colorTexture), viewportSize);
    
    DrawControls();
    ImGui::End();
    
    // Reset OpenGL state
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
}

void GeometryViewerView::EnsureFramebuffer(float width, float height) {
    // If framebuffer already exists with correct size, return
    if (m_framebuffer && m_colorTexture && m_depthTexture) {
        GLint texWidth, texHeight;
        glBindTexture(GL_TEXTURE_2D, m_colorTexture);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texWidth);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texHeight);
        
        if (texWidth == (GLint)width && texHeight == (GLint)height) {
            return;
        }
    }
    
    // Clean up existing framebuffer if it exists
    if (m_framebuffer) {
        glDeleteFramebuffers(1, &m_framebuffer);
        glDeleteTextures(1, &m_colorTexture);
        glDeleteTextures(1, &m_depthTexture);
    }
    
    // Create framebuffer
    glGenFramebuffers(1, &m_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    
    // Create color attachment texture
    glGenTextures(1, &m_colorTexture);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);
    
    // Create depth texture
    glGenTextures(1, &m_depthTexture);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, (GLsizei)width, (GLsizei)height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);
    
    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        // Handle error
    }
}