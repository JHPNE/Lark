#include "PrimitiveMeshSelectionView.h"

#include "Components/Geometry.h"
#include "Geometry/Geometry.h"
#include "Project/Project.h"
#include "Project/Scene.h"
#include "View/GeometryViewerView.h"
#include "View/Style.h"
#include <imgui.h>

void PrimitiveMeshSelectionView::SetActiveProject(std::shared_ptr<Project> activeProject)
{
    m_project = activeProject;
}

void PrimitiveMeshSelectionView::SetSegments(uint32_t segments[3])
{
    m_segments[0] = segments[0];
    m_segments[1] = segments[1];
    m_segments[2] = segments[2];
}

void PrimitiveMeshSelectionView::Draw()
{
    if (!m_project)
        return;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    window_flags |= ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("Primitive Mesh Creator", nullptr, window_flags);
    DrawWindowGradientBackground(ImVec4(0.10f, 0.10f, 0.13f, 0.30f),
                                 ImVec4(0.10f, 0.10f, 0.13f, 0.80f));

    ImGui::Text("Primitive Mesh Settings");
    ImGui::Separator();

    std::shared_ptr<Scene> activeScene = m_project->GetActiveScene();

    if (!activeScene)
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No active scene");
        ImGui::End();
        return;
    }

    // Mesh Type Selection
    ImGui::Text("Mesh Type:");

    const char *meshTypes[] = {"Cube", "UV Sphere", "Cylinder"};

    if (ImGui::Combo("##MeshType", &m_selectedMeshIndex, meshTypes, IM_ARRAYSIZE(meshTypes)))
    {
        // Convert combo index to enum and reset segments
        switch (m_selectedMeshIndex)
        {
        case 0:
            m_selectedMesh = content_tools::PrimitiveMeshType::cube;
            m_segments[0] = m_segments[1] = m_segments[2] = 1;
            break;
        case 1:
            m_selectedMesh = content_tools::PrimitiveMeshType::uv_sphere;
            m_segments[0] = 32; // Longitude segments
            m_segments[1] = 16; // Latitude segments
            m_segments[2] = 1;
            break;
        case 2:
            m_selectedMesh = content_tools::PrimitiveMeshType::cylinder;
            m_segments[0] = 32; // Radial segments
            m_segments[1] = 1;  // Height segments
            m_segments[2] = 1;  // Cap segments
            break;
        }
    }

    ImGui::Spacing();

    // Segment Controls (vary based on mesh type)
    ImGui::Text("Segments:");

    switch (m_selectedMeshIndex)
    {
    case 0: // Cube
    {
        // Convert uint32_t to int for ImGui, then back
        int segments[3] = {static_cast<int>(m_segments[0]), static_cast<int>(m_segments[1]),
                           static_cast<int>(m_segments[2])};
        if (ImGui::DragInt3("X/Y/Z##Segments", segments, 1, 1, 10))
        {
            m_segments[0] = static_cast<uint32_t>(std::max(1, segments[0]));
            m_segments[1] = static_cast<uint32_t>(std::max(1, segments[1]));
            m_segments[2] = static_cast<uint32_t>(std::max(1, segments[2]));
        }
    }
    break;

    case 1: // UV Sphere
    {
        int longitude = static_cast<int>(m_segments[0]);
        int latitude = static_cast<int>(m_segments[1]);

        if (ImGui::DragInt("Longitude##Segments", &longitude, 1, 8, 64))
        {
            m_segments[0] = static_cast<uint32_t>(std::max(8, std::min(64, longitude)));
        }
        if (ImGui::DragInt("Latitude##Segments", &latitude, 1, 4, 32))
        {
            m_segments[1] = static_cast<uint32_t>(std::max(4, std::min(32, latitude)));
        }
    }
    break;

    case 2: // Cylinder
    {
        int radial = static_cast<int>(m_segments[0]);
        int height = static_cast<int>(m_segments[1]);
        int cap = static_cast<int>(m_segments[2]);

        if (ImGui::DragInt("Radial##Segments", &radial, 1, 8, 64))
        {
            m_segments[0] = static_cast<uint32_t>(std::max(8, std::min(64, radial)));
        }
        if (ImGui::DragInt("Height##Segments", &height, 1, 1, 10))
        {
            m_segments[1] = static_cast<uint32_t>(std::max(1, std::min(10, height)));
        }
        if (ImGui::DragInt("Cap##Segments", &cap, 1, 1, 5))
        {
            m_segments[2] = static_cast<uint32_t>(std::max(1, std::min(5, cap)));
        }
    }
    break;
    }

    ImGui::Spacing();

    // Size Controls
    ImGui::Text("Size:");
    ImGui::DragFloat3("##Size", m_size, 0.1f, 0.1f, 10.0f);

    ImGui::Spacing();

    // LOD Level
    ImGui::Text("LOD Level:");
    int lod = static_cast<int>(m_lod);
    if (ImGui::SliderInt("##LOD", &lod, 0, 4))
    {
        m_lod = static_cast<uint32_t>(lod);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Create Button
    if (ImGui::Button("Create Mesh", ImVec2(-1, 30)))
    {
        CreatePrimitiveMesh();
    }

    ImGui::End();
}

void PrimitiveMeshSelectionView::CreatePrimitiveMesh()
{
    if (!m_project)
        return;

    auto activeScene = m_project->GetActiveScene();
    if (!activeScene)
    {
        Logger::Get().Log(MessageType::Warning, "No active scene to create mesh in");
        return;
    }

    // Generate mesh name based on type
    std::string meshName;
    switch (m_selectedMesh)
    {
    case content_tools::PrimitiveMeshType::cube:
        meshName = "Cube";
        break;
    case content_tools::PrimitiveMeshType::uv_sphere:
        meshName = "Sphere";
        break;
    case content_tools::PrimitiveMeshType::cylinder:
        meshName = "Cylinder";
        break;
    default:
        meshName = "Primitive";
        break;
    }

    // Add a unique suffix based on entity count
    static int meshCounter = 0;
    meshName += "_" + std::to_string(++meshCounter);

    // Create the primitive geometry
    auto geometry =
        lark::editor::Geometry::CreatePrimitive(m_selectedMesh, m_size, m_segments, m_lod);

    // Setup geometry initializer
    GeometryInitializer geomInit;
    geomInit.geometryName = meshName;
    geomInit.geometryType = GeometryType::PrimitiveType;
    geomInit.visible = true;
    geomInit.meshType = m_selectedMesh;

    // Create entity with geometry using the specified approach
    auto entity = activeScene->CreateEntityInternal(meshName);

    if (entity)
    {
        // Add geometry component
        auto *geomComponent = entity->AddComponent<Geometry>(&geomInit);

        // Set the scene data on the geometry component
        if (geomComponent && geometry->GetScene())
        {
            geomComponent->SetScene(*geometry->GetScene());
        }

        // Update the entity in the scene
        activeScene->UpdateEntity(entity->GetID());

        // Add to geometry viewer
        GeometryViewerView::Get().AddGeometry(entity->GetID());

        // Log success
        m_lastCreatedName = meshName;
    }
}