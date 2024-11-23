#include "GeometryRenderer.h"

GLuint GeometryRenderer::m_basicShader = 0;

bool GeometryRenderer::Initialize() {
    // Create and compile shaders
    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, s_basicVertexShader);
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, s_basicFragmentShader);

    if (!vertexShader || !fragmentShader) return false;

    // Create shader program
    m_basicShader = glCreateProgram();
    glAttachShader(m_basicShader, vertexShader);
    glAttachShader(m_basicShader, fragmentShader);
    glLinkProgram(m_basicShader);

    // Check for linking errors
    GLint success;
    glGetProgramiv(m_basicShader, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_basicShader, 512, nullptr, infoLog);
        // Handle error
        return false;
    }

    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
};

void GeometryRenderer::Shutdown() {
    if (m_basicShader) {
        glDeleteProgram(m_basicShader);
        m_basicShader = 0;
    }
}

std::unique_ptr<GeometryRenderer::LODGroupBuffers> GeometryRenderer::CreateBuffersFromGeometry(drosim::editor::Geometry* geometry) {
    if (!geometry) return nullptr;

    auto groupBuffers = std::make_unique<LODGroupBuffers>();

    // Process each LOD group
    if (auto* lodGroup = geometry->GetLODGroup(0)) {
        groupBuffers->name = lodGroup->name;


        // Process each LOD level
        for (const auto& lod : lodGroup->lods) {
            auto lodBuffers = std::make_shared<LODLevelBuffers>();
            lodBuffers->name = lod->name;
            lodBuffers->threshold = lod->lod_threshold;

            // Process each mesh in this LOD level
            for (const auto& mesh : lod->meshes) {
                auto meshBuffers = CreateMeshBuffers(mesh);
                if (meshBuffers) {
                    lodBuffers->meshBuffers.push_back(std::move(meshBuffers));
                }
            }

            groupBuffers->lodLevels.push_back(lodBuffers);
        }
    }

    return groupBuffers;
}

void GeometryRenderer::RenderGeometryAtLOD(const LODGroupBuffers* groupBuffers,
                             const glm::mat4& view,
                             const glm::mat4& projection,
                             float distanceToCamera) {
    if (!groupBuffers) return;

    // Use shader program
    glUseProgram(m_basicShader);

    // Set uniforms
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(m_basicShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(m_basicShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_basicShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Set lighting uniforms
    glm::vec3 lightPos(5.0f, 5.0f, 5.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    glm::vec3 objectColor(0.5f, 0.5f, 1.0f);

    glUniform3fv(glGetUniformLocation(m_basicShader, "lightPos"), 1, glm::value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(m_basicShader, "lightColor"), 1, glm::value_ptr(lightColor));
    glUniform3fv(glGetUniformLocation(m_basicShader, "objectColor"), 1, glm::value_ptr(objectColor));


    const LODLevelBuffers* selectedLOD = nullptr;

    for (const auto& lod : groupBuffers->lodLevels) {
        if (lod->threshold <= distanceToCamera) {
            selectedLOD = lod.get();
            break;
        }
    }

    // if no LOD level selected, use the last one
    if (!selectedLOD && !groupBuffers->lodLevels.empty()) {
        selectedLOD = groupBuffers->lodLevels.back().get();
    }

    // Render all meshes in the selected LOD level
    if (selectedLOD) {
        for (const auto& meshBuffers : selectedLOD->meshBuffers) {
            RenderMesh(meshBuffers.get());
        }
    }
};

std::shared_ptr<GeometryRenderer::MeshBuffers> GeometryRenderer::CreateMeshBuffers(const std::shared_ptr<drosim::editor::Mesh>& mesh) {
    if (!mesh || mesh->vertices.empty() || mesh->indices.empty()) return nullptr;

    auto buffers = std::make_shared<MeshBuffers>();

    // Create and bind VAO
    glGenVertexArrays(1, &buffers->vao);
    glBindVertexArray(buffers->vao);

    // Create and bind VBO
    glGenBuffers(1, &buffers->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, buffers->vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh->vertices.size(), mesh->vertices.data(), GL_STATIC_DRAW);

    // Create and bind IBO
    glGenBuffers(1, &buffers->ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indices.size(), mesh->indices.data(), GL_STATIC_DRAW);

    // Store index count
    buffers->indexCount = mesh->index_count;

    // Set up vertex attributes based on vertex format
    size_t offset = 0;
    const int stride = mesh->vertex_size;

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
    offset += sizeof(float) * 3;

    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
    offset += sizeof(float) * 3;

    // UV (assuming float[2])
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offset);

    // Unbind VAO
    glBindVertexArray(0);

    return buffers;
};

GLuint GeometryRenderer::CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        return 0;
    }
    return shader;
}
void GeometryRenderer::RenderMesh(const MeshBuffers* meshBuffers) {
    if (!meshBuffers || !meshBuffers->vao) return;

    glBindVertexArray(meshBuffers->vao);
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(meshBuffers->indexCount),
                   GL_UNSIGNED_INT,
                   0);
    glBindVertexArray(0);
}
