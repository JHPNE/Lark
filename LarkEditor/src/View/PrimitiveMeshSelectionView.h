#pragma once
#include "EngineAPI.h"
#include <memory>
#include <string>

class Project;

class PrimitiveMeshSelectionView
{
  public:
    static PrimitiveMeshSelectionView &Get()
    {
        static PrimitiveMeshSelectionView instance;
        return instance;
    }

    void Draw();
    void SetActiveProject(std::shared_ptr<Project> activeProject);
    bool &GetShowState() { return m_show; }

  private:
    PrimitiveMeshSelectionView() = default;
    ~PrimitiveMeshSelectionView() = default;

    // Prevent copying
    PrimitiveMeshSelectionView(const PrimitiveMeshSelectionView &) = delete;
    PrimitiveMeshSelectionView &operator=(const PrimitiveMeshSelectionView &) = delete;

    void SetSegments(uint32_t segments[3]);
    void CreatePrimitiveMesh();

    // UI State
    bool m_show = true;
    std::shared_ptr<Project> m_project;

    // Mesh creation parameters
    content_tools::PrimitiveMeshType m_selectedMesh = content_tools::PrimitiveMeshType::cube;
    int m_selectedMeshIndex = 0;          // Track combo index separately
    uint32_t m_segments[3] = {1, 1, 1};   // Default segments (uint32_t)
    float m_size[3] = {1.0f, 1.0f, 1.0f}; // Default size
    uint32_t m_lod = 0;                   // Default LOD level (uint32_t)

    // Last created mesh name for feedback
    std::string m_lastCreatedName;
};