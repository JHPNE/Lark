#pragma once
#include "ObservableProperty.h"
#include "../Commands/ICommand.h"
#include "../Models/GeometryModel.h"
#include "../Services/GeometryService.h"
#include "../Rendering/GeometryRenderManager.h"
#include "../Project/Project.h"
#include "../Components/Geometry.h"
#include <memory>
#include <vector>
#include <glm/gtx/matrix_decompose.hpp>
#include "../Services/SelectionService.h"
#include "Services/TransformService.h"

#include <Services/EventBus.h>

class GeometryViewModel
{
public:
    // Observable properties for UI
    ObservableProperty<uint32_t> SelectedEntityId{static_cast<uint32_t>(-1)};
    ObservableProperty<bool> HasSelection{false};
    ObservableProperty<std::string> StatusMessage{""};

    // Camera properties
    ObservableProperty<glm::vec3> CameraPosition{0.0f, 0.0f, 0.0f};
    ObservableProperty<glm::vec3> CameraRotation{0.0f, 0.0f, 0.0f};
    ObservableProperty<float> CameraDistance{10.0f};

    // Gizmo properties
    ObservableProperty<int> GizmoOperation{0}; // 0=Translate, 1=Rotate, 2=Scale
    ObservableProperty<bool> IsUsingGizmo{false};

    // Primitive creation properties
    ObservableProperty<int> PrimitiveType{0}; // 0=Cube, 1=Sphere, 2=Cylinder
    ObservableProperty<glm::vec3> PrimitiveSize{1.0f, 1.0f, 1.0f};
    ObservableProperty<glm::ivec3> PrimitiveSegments{1, 1, 1};
    ObservableProperty<int> PrimitiveLOD{0};

    // Commands
    std::unique_ptr<RelayCommand<>> CreatePrimitiveCommand;
    std::unique_ptr<RelayCommand<std::string>> LoadGeometryCommand;
    std::unique_ptr<RelayCommand<uint32_t>> RemoveGeometryCommand;
    std::unique_ptr<RelayCommand<>> ResetCameraCommand;
    std::unique_ptr<RelayCommand<>> RandomizeVerticesCommand;
    std::unique_ptr<RelayCommand<uint32_t>> SelectEntityCommand;

    GeometryViewModel()
        : m_model(std::make_unique<GeometryModel>())
        , m_renderManager(std::make_unique<GeometryRenderManager>())
        , m_service(GeometryService::Get())
        , m_transformService(TransformService::Get())
    {
        InitializeCommands();
        SubscribeToSelectionService();
        SubscribeToPropertyChanges();
        SubscribeToEvents();
    }

    ~GeometryViewModel() = default;

    void SetProject(std::shared_ptr<Project> project)
    {
        if (m_project != project)
        {
            ClearAll();
            m_project = project;
            if (m_project && m_project->GetActiveScene())
            {
                LoadExistingGeometries();
                printf("Loaded %zu geometries from project\n", m_model->GetAllGeometries().size());
            }
            else
            {
                printf("Warning: Project set but no active scene available\n");
            }
        }
    }

    // Get the geometry model for queries
    const GeometryModel& GetModel() const { return *m_model; }

    // Get render manager for rendering
    GeometryRenderManager& GetRenderManager() {
        if (m_renderManager != nullptr) {
            return *m_renderManager;
        } else {
            throw std::runtime_error("RenderManager is null!");
        }
    }

    // Add geometry from existing entity
    bool AddGeometryFromEntity(std::shared_ptr<GameEntity> entity)
    {
        if (!entity)
            return false;

        auto* geomComponent = entity->GetComponent<Geometry>();
        if (!geomComponent)
        {
            UpdateStatus("Entity has no geometry component");
            return false;
        }

        auto instance = std::make_unique<GeometryInstance>();
        instance->entityId = entity->GetID();
        instance->name = geomComponent->GetGeometryName();
        instance->type = geomComponent->GetGeometryType();
        instance->visible = geomComponent->IsVisible();

        // Get scene data from component
        if (auto* scene = geomComponent->GetScene())
        {
            instance->sceneData = *scene;

            // Create render buffers
            if (!m_renderManager->CreateOrUpdateBuffers(entity->GetID(), scene))
            {
                UpdateStatus("Failed to create render buffers");
                return false;
            }
        }

        m_model->AddGeometry(entity->GetID(), std::move(instance));
        UpdateStatus("Added geometry: " + entity->GetName());
        return true;
    }

    // Update geometry from engine modifications
    void UpdateGeometryFromEngine(uint32_t entityId)
    {
        content_tools::SceneData sceneData{};

        if (!m_service.GetModifiedMeshData(entityId, &sceneData))
        {
            UpdateStatus("Failed to get modified mesh data");
            return;
        }

        if (!sceneData.buffer || sceneData.buffer_size == 0)
        {
            UpdateStatus("No mesh data received");
            return;
        }

        // Create new geometry from data
        auto geometry = std::make_unique<lark::editor::Geometry>();
        if (!geometry->FromRawData(sceneData.buffer, sceneData.buffer_size))
        {
            UpdateStatus("Failed to parse mesh data");
            free(sceneData.buffer);
            return;
        }

        free(sceneData.buffer);

        // Update model
        if (auto* scene = geometry->GetScene())
        {
            m_model->UpdateGeometryData(entityId, scene);

            // Update render buffers
            m_renderManager->CreateOrUpdateBuffers(entityId, scene);

            // Update component in entity
            UpdateEntityComponent(entityId, scene);
        }

        UpdateStatus("Updated geometry for entity " + std::to_string(entityId));
    }

    // Get entity transform
    glm::mat4 GetEntityTransform(uint32_t entityId)
    {
        return m_service.GetEntityTransform(entityId);
    }

    // Update transform from gizmo
    void UpdateTransformFromGizmo(uint32_t entityId, const float* matrix)
    {
        if (!matrix) return;

        TransformData transformData = TransformService::Get().DecomposeMatrix(matrix);

        if (m_project)
        {
            if (auto scene = m_project->GetActiveScene())
            {
                if (auto entity = scene->GetEntity(entityId))
                {
                    TransformService::Get().UpdateEntityTransform(entity, transformData);
                }
            }
        }

    }

private:
    std::unique_ptr<GeometryModel> m_model;
    std::unique_ptr<GeometryRenderManager> m_renderManager;
    GeometryService& m_service;
    std::shared_ptr<Project> m_project;
    TransformService& m_transformService;

    void InitializeCommands()
    {
        // Create primitive command
        CreatePrimitiveCommand = std::make_unique<RelayCommand<>>(
            [this]() { ExecuteCreatePrimitive(); },
            [this]() { return m_project && m_project->GetActiveScene(); }
        );

        // Load geometry command
        LoadGeometryCommand = std::make_unique<RelayCommand<std::string>>(
            [this](const std::string& path) { ExecuteLoadGeometry(path); },
            [this](const std::string&) { return m_project && m_project->GetActiveScene(); }
        );

        // Remove geometry command
        RemoveGeometryCommand = std::make_unique<RelayCommand<uint32_t>>(
            [this](uint32_t id) { ExecuteRemoveGeometry(id); },
            [this](uint32_t id) { return m_model->HasGeometry(id); }
        );

        // Reset camera command
        ResetCameraCommand = std::make_unique<RelayCommand<>>(
            [this]() { ExecuteResetCamera(); }
        );

        // Randomize vertices command
        RandomizeVerticesCommand = std::make_unique<RelayCommand<>>(
            [this]() { ExecuteRandomizeVertices(); },
            [this]() { return HasSelection.Get() && SelectedEntityId.Get() != static_cast<uint32_t>(-1); }
        );

        // Select entity command
        SelectEntityCommand = std::make_unique<RelayCommand<uint32_t>>(
            [this](uint32_t id) { ExecuteSelectEntity(id); },
            [this](uint32_t id) { return m_model->HasGeometry(id); }
        );
    }

    void SubscribeToEvents()
    {
        EventBus::Get().Subscribe<EntityRemovedEvent>(
            [this](const EntityRemovedEvent& e) {
                    ExecuteRemoveGeometry(e.entityId);
            }
        );

        EventBus::Get().Subscribe<SceneChangedEvent>(
            [this](const SceneChangedEvent& e) {
                // When this happens dont render entities in a different scene
                    HandleNonActiveSceneGeometry(e.sceneId);
            }
        );

        EventBus::Get().Subscribe<GeometryVisibilityChangedEvent>(
            [this](const GeometryVisibilityChangedEvent& e) {
                HandleGeometryVisibilityChanged(e.entityId, e.visible);
            }
        );

    }

    void HandleGeometryVisibilityChanged(uint32_t entityId, bool visible)
    {
        // Update render manager visibility
        m_renderManager->SetVisible(entityId, visible);

        // Update model if needed
        if (auto* geom = m_model->GetGeometry(entityId))
        {
            geom->visible = visible;
        }

        UpdateStatus("Updated visibility for entity " + std::to_string(entityId));
    }

    void HandleNonActiveSceneGeometry(uint32_t scene_id)
    {
        for (auto scene : m_project->GetScenes())
        {
            for (auto entity : scene->GetEntities())
            {
                m_renderManager->SetVisible(entity->GetID(), scene->GetID() == scene_id);
            }
        }
    }

    void SubscribeToSelectionService()
    {
        auto& selectionService = SelectionService::Get();

        selectionService.SubscribeToSelectionChange(
            [this](uint32_t oldId, uint32_t newId) {
                SelectedEntityId = newId;
                HasSelection = (newId != static_cast<uint32_t>(-1));
            }
        );
    }

    void SubscribeToPropertyChanges()
    {
        CameraDistance.Subscribe([this](const float& old, const float& value) {
            // Clamp camera distance
            if (value < 0.1f) CameraDistance = 0.1f;
            if (value > 100.0f) CameraDistance = 100.0f;
        });
    }

    void ExecuteCreatePrimitive()
    {
        if (!m_project || !m_project->GetActiveScene())
            return;

        // Map UI index to primitive type
        content_tools::PrimitiveMeshType meshType;
        switch (PrimitiveType.Get())
        {
            case 0: meshType = content_tools::PrimitiveMeshType::cube; break;
            case 1: meshType = content_tools::PrimitiveMeshType::uv_sphere; break;
            case 2: meshType = content_tools::PrimitiveMeshType::cylinder; break;
            default: meshType = content_tools::PrimitiveMeshType::cube; break;
        }

        // Get segments
        glm::ivec3 segs = PrimitiveSegments.Get();
        uint32_t segments[3] = {
            static_cast<uint32_t>(segs.x),
            static_cast<uint32_t>(segs.y),
            static_cast<uint32_t>(segs.z)
        };

        // Create geometry instance
        auto instance = m_service.CreatePrimitive(
            meshType, PrimitiveSize.Get(), segments, PrimitiveLOD.Get());

        if (!instance)
        {
            UpdateStatus("Failed to create primitive");
            return;
        }

        // Create entity
        auto scene = m_project->GetActiveScene();
        std::string name = GetPrimitiveName(meshType);
        auto entity = scene->CreateEntity(name);

        if (!entity)
        {
            UpdateStatus("Failed to create entity");
            return;
        }

        // Setup geometry component
        GeometryInitializer geomInit;
        geomInit.geometryName = name;
        geomInit.geometryType = GeometryType::PrimitiveType;
        geomInit.visible = true;
        geomInit.meshType = meshType;

        auto* geomComponent = entity->AddComponent<Geometry>(&geomInit);

        content_tools::scene* sceneDataPtr = nullptr;
        if (geomComponent && instance->geometryData->GetScene())
        {
            geomComponent->SetScene(*instance->geometryData->GetScene());
            sceneDataPtr = geomComponent->GetScene(); // Get pointer from component
        }

        // Update entity in scene
        scene->UpdateEntity(entity->GetID());

        // Add to model
        instance->visible = true;
        instance->entityId = entity->GetID();
        instance->name = name;

        m_model->AddGeometry(entity->GetID(), std::move(instance));

        // Create render buffers
        if (sceneDataPtr)
        {
            if (!m_renderManager->CreateOrUpdateBuffers(entity->GetID(), sceneDataPtr))
            {
                UpdateStatus("Failed to create render buffers for: " + name);
                Logger::Get().Log(MessageType::Error, "Failed to create render buffers");
                return;
            }

            m_renderManager->SetVisible(entity->GetID(), true);
        }
        else
        {
            UpdateStatus("No scene data available for: " + name);
            Logger::Get().Log(MessageType::Error, "No scene data for geometry");
            return;
        }

        // Event to notify SceneViewModel
        EntityCreatedEvent event;
        event.entityId = entity->GetID();
        event.sceneId = scene->GetID();
        event.entityName = name;
        EventBus::Get().Publish(event);

        UpdateStatus("Created primitive: " + name);
        Logger::Get().Log(MessageType::Info, "Created primitive geometry: " + name);
    }

    void ExecuteLoadGeometry(const std::string& filepath)
    {
        if (!m_project || !m_project->GetActiveScene())
            return;

        // Load geometry
        auto instance = m_service.LoadFromFile(filepath);
        if (!instance)
        {
            UpdateStatus("Failed to load geometry from: " + filepath);
            return;
        }

        // Create entity
        auto scene = m_project->GetActiveScene();
        std::string name = fs::path(filepath).stem().string();
        auto entity = scene->CreateEntityInternal(name);

        if (!entity)
        {
            UpdateStatus("Failed to create entity");
            return;
        }

        // Setup geometry component
        GeometryInitializer geomInit;
        geomInit.geometryName = name;
        geomInit.geometryType = GeometryType::ObjImport;
        geomInit.visible = true;
        geomInit.geometrySource = filepath;

        auto* geomComponent = entity->AddComponent<Geometry>(&geomInit);
        if (geomComponent && instance->geometryData->GetScene())
        {
            geomComponent->SetScene(*instance->geometryData->GetScene());
        }

        // Update entity in scene
        scene->UpdateEntity(entity->GetID());

        // Add to model
        instance->visible = true;
        instance->entityId = entity->GetID();
        instance->name = name;

        m_model->AddGeometry(entity->GetID(), std::move(instance));

        // Create render buffers
        if (auto* geom = m_model->GetGeometry(entity->GetID()))
        {
            m_renderManager->CreateOrUpdateBuffers(entity->GetID(), &geom->sceneData);
        }

        UpdateStatus("Loaded geometry: " + name);
        Logger::Get().Log(MessageType::Info, "Loaded geometry from: " + filepath);
    }

    void ExecuteRemoveGeometry(uint32_t entityId)
    {
        // Remove from render manager
        m_renderManager->RemoveBuffers(entityId);

        // Remove from model
        m_model->RemoveGeometry(entityId);

        // If this was selected, clear selection
        if (SelectedEntityId.Get() == entityId)
        {
            SelectedEntityId = static_cast<uint32_t>(-1);
        }

        UpdateStatus("Removed geometry for entity " + std::to_string(entityId));
    }

    void ExecuteSelectEntity(uint32_t entityId)
    {
        SelectionService::Get().SelectEntity(entityId);
        UpdateStatus("Selected entity " + std::to_string(entityId));
    }

    void ExecuteResetCamera()
    {
        CameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);
        CameraRotation = glm::vec3(0.0f, 0.0f, 0.0f);
        CameraDistance = 10.0f;
        UpdateStatus("Camera reset");
    }

    void ExecuteRandomizeVertices()
    {
        uint32_t entityId = SelectedEntityId.Get();
        if (entityId == static_cast<uint32_t>(-1))
            return;

        auto* geom = m_model->GetGeometry(entityId);
        if (!geom || geom->sceneData.lod_groups.empty())
            return;

        // Get vertices from first mesh
        const auto& mesh = geom->sceneData.lod_groups[0].meshes[0];
        std::vector<glm::vec3> vertices;
        vertices.reserve(mesh.vertices.size());

        // Randomize vertices
        for (const auto& v : mesh.vertices)
        {
            glm::vec3 pos = v.position;
            pos.x += ((float)rand() / RAND_MAX) * 0.5f - 0.25f;
            pos.y += ((float)rand() / RAND_MAX) * 0.5f - 0.25f;
            pos.z += ((float)rand() / RAND_MAX) * 0.5f - 0.25f;
            vertices.push_back(pos);
        }

        // Send to engine
        m_service.ModifyVertexPositions(entityId, vertices);

        // Update from engine
        UpdateGeometryFromEngine(entityId);

        UpdateStatus("Randomized vertices for entity " + std::to_string(entityId));
    }

    void LoadExistingGeometries()
    {
        if (!m_project)
        {
            printf("LoadExistingGeometries: No project\n");
            return;
        }

        auto scene = m_project->GetActiveScene();
        if (!scene)
        {
            printf("LoadExistingGeometries: No active scene\n");
            return;
        }

        printf("LoadExistingGeometries: Processing %zu entities\n", scene->GetEntities().size());

        for (const auto& entity : scene->GetEntities())
        {
            if (entity->GetComponent<Geometry>())
            {
                printf("Found geometry component on entity: %s (ID: %u)\n",
                   entity->GetName().c_str(), entity->GetID());

                if (!AddGeometryFromEntity(entity))
                {
                    printf("Failed to add geometry for entity: %s\n", entity->GetName().c_str());
                }
            }
        }

        printf("LoadExistingGeometries complete: %zu geometries loaded\n",
                   m_model->GetAllGeometries().size());
        UpdateStatus("Loaded existing geometries");
    }

    void ClearAll()
    {
        m_renderManager->ClearAll();
        m_model->Clear();
        SelectedEntityId = static_cast<uint32_t>(-1);
    }

    void UpdateEntityComponent(uint32_t entityId, content_tools::scene* scene)
    {
        if (!m_project || !scene)
            return;

        auto activeScene = m_project->GetActiveScene();
        if (!activeScene)
            return;

        auto entity = activeScene->GetEntity(entityId);
        if (!entity)
            return;

        auto* geomComponent = entity->GetComponent<Geometry>();
        if (geomComponent)
        {
            geomComponent->SetScene(*scene);
        }
    }

    std::string GetPrimitiveName(content_tools::PrimitiveMeshType type)
    {
        static int counter = 0;
        std::string base;

        switch (type)
        {
            case content_tools::PrimitiveMeshType::cube: base = "Cube"; break;
            case content_tools::PrimitiveMeshType::uv_sphere: base = "Sphere"; break;
            case content_tools::PrimitiveMeshType::cylinder: base = "Cylinder"; break;
            default: base = "Primitive"; break;
        }

        return base + "_" + std::to_string(++counter);
    }

    void UpdateStatus(const std::string& message)
    {
        StatusMessage = message;
    }
};
