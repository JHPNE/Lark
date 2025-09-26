#pragma once
#include "ObservableProperty.h"
#include "../Commands/ICommand.h"
#include "../Project/Project.h"
#include "../Services/GeometryService.h"
#include "../Utils/Etc/Logger.h"
#include <memory>
#include <glm/glm.hpp>
#include "../Services/ProjectSettings.h"
#include "Services/EventBus.h"
#include "Services/PhysicService.h"

class ProjectSettingsViewModel
{
public:
    // Camera settings
    ObservableProperty<CameraSettings> Camera;

    // World physics settings
    ObservableProperty<WorldSettings> World;

    // Render settings
    ObservableProperty<RenderSettings> Render;

    // Geometry creation settings
    ObservableProperty<int> PrimitiveType{0}; // 0=Cube, 1=Sphere, 2=Cylinder
    ObservableProperty<glm::vec3> PrimitiveSize{1.0f, 1.0f, 1.0f};
    ObservableProperty<glm::ivec3> PrimitiveSegments{1, 1, 1};
    ObservableProperty<int> PrimitiveLOD{0};

    // UI State
    ObservableProperty<int> ActiveTab{0};
    ObservableProperty<std::string> StatusMessage{""};
    ObservableProperty<bool> HasProject{false};

    // Commands
    std::unique_ptr<RelayCommand<>> SaveSettingsCommand;
    std::unique_ptr<RelayCommand<>> LoadSettingsCommand;
    std::unique_ptr<RelayCommand<>> ResetCameraCommand;
    std::unique_ptr<RelayCommand<>> ApplyWorldSettingsCommand;
    std::unique_ptr<RelayCommand<>> CreatePrimitiveCommand;
    std::unique_ptr<RelayCommand<std::string>> LoadGeometryCommand;

    ProjectSettingsViewModel()
        : m_service(PhysicService::Get())
    {
        InitializeCommands();
        SetDefaultValues();
    }

    void SetProject(std::shared_ptr<Project> project)
    {
        if (m_project != project)
        {
            m_project = project;
            HasProject = (project != nullptr);

            if (project)
            {
                LoadSettings();
            }
            else
            {
                // No project, use defaults
                SetDefaultValues();
            }
        }
    }

    glm::mat4 GetViewMatrix() const
    {
        const auto& cam = Camera.Get();

        glm::vec3 forward(0.0f, 0.0f, -1.0f);
        glm::vec3 up(0.0f, 1.0f, 0.0f);

        glm::mat4 rotation = glm::mat4(1.0f);
        rotation = glm::rotate(rotation, glm::radians(cam.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        rotation = glm::rotate(rotation, glm::radians(cam.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        rotation = glm::rotate(rotation, glm::radians(cam.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

        forward = glm::vec3(rotation * glm::vec4(forward, 0.0f));
        up = glm::vec3(rotation * glm::vec4(up, 0.0f));

        glm::vec3 actualCameraPos = cam.position - (forward * cam.distance);
        return glm::lookAt(actualCameraPos, cam.position, up);
    }

    glm::mat4 GetProjectionMatrix(float aspectRatio) const
    {
        const auto& cam = Camera.Get();
        return glm::perspective(glm::radians(cam.fov), aspectRatio, cam.nearPlane, cam.farPlane);
    }

private:
    std::shared_ptr<Project> m_project;
    PhysicService& m_service;

    void InitializeCommands()
    {
        SaveSettingsCommand = std::make_unique<RelayCommand<>>(
            [this]() { SaveSettings(); },
            [this]() { return HasProject.Get(); }
        );

        LoadSettingsCommand = std::make_unique<RelayCommand<>>(
            [this]() { LoadSettings(); },
            [this]() { return HasProject.Get(); }
        );

        ResetCameraCommand = std::make_unique<RelayCommand<>>(
            [this]() {
                CameraSettings defaultCam;
                Camera = defaultCam;
                UpdateStatus("Camera reset to defaults");
            }
        );

        ApplyWorldSettingsCommand = std::make_unique<RelayCommand<>>(
            [this]() {
                ApplyWorldSettingsToEngine();
                UpdateStatus("World settings applied");
            }
        );

        CreatePrimitiveCommand = std::make_unique<RelayCommand<>>(
            [this]() { ExecuteCreatePrimitive(); },
            [this]() { return HasProject.Get() && m_project->GetActiveScene(); }
        );

        LoadGeometryCommand = std::make_unique<RelayCommand<std::string>>(
            [this](const std::string& path) { ExecuteLoadGeometry(path); },
            [this](const std::string&) { return HasProject.Get() && m_project->GetActiveScene(); }
        );
    }

    void SetDefaultValues()
    {
        CameraSettings defaultCam;
        Camera = defaultCam;

        WorldSettings defaultWorld;
        World = defaultWorld;

        RenderSettings defaultRender;
        Render = defaultRender;
    }

    // Not in our Project serilization/save etc since we might want some import/export later inbetween projects
    void SaveSettings()
    {
        if (!m_project) return;

        // Save to project directory as .settings file
        auto settingsPath = m_project->GetPath() / (m_project->GetName() + ".settings");

        tinyxml2::XMLDocument doc;
        SerializationContext context(doc);

        auto root = doc.NewElement("ProjectSettings");
        doc.LinkEndChild(root);

        auto cameraElement = doc.NewElement("Camera");
        Camera.Get().Serialize(cameraElement, context);
        root->LinkEndChild(cameraElement);

        auto worldElement = doc.NewElement("World");
        World.Get().Serialize(worldElement, context);
        root->LinkEndChild(worldElement);

        auto renderElement = doc.NewElement("Render");
        Render.Get().Serialize(renderElement, context);
        root->LinkEndChild(renderElement);

        if (doc.SaveFile(settingsPath.string().c_str()) == tinyxml2::XML_SUCCESS)
        {
            UpdateStatus("Settings saved");
            Logger::Get().Log(MessageType::Info, "Project settings saved to: " + settingsPath.string());
        }
        else
        {
            UpdateStatus("Failed to save settings");
            Logger::Get().Log(MessageType::Error, "Failed to save project settings");
        }
    }

    void LoadSettings()
    {
        if (!m_project)
        {
            SetDefaultValues();
            return;
        }

        auto settingsPath = m_project->GetPath() / (m_project->GetName() + ".settings");

        if (!fs::exists(settingsPath))
        {
            SetDefaultValues();
            return;
        }

        tinyxml2::XMLDocument doc;
        if (doc.LoadFile(settingsPath.string().c_str()) != tinyxml2::XML_SUCCESS)
        {
            SetDefaultValues();
            return;
        }

        SerializationContext context(doc);

        auto root = doc.FirstChildElement("ProjectSettings");
        if (!root)
        {
            SetDefaultValues();
            return;
        }

        CameraSettings cam;
        if (auto cameraElement = root->FirstChildElement("Camera"))
        {
            cam.Deserialize(cameraElement, context);
        }
        Camera = cam;

        WorldSettings world;
        if (auto worldElement = root->FirstChildElement("World"))
        {
            world.Deserialize(worldElement, context);
        }
        World = world;

        RenderSettings render;
        if (auto renderElement = root->FirstChildElement("Render"))
        {
            render.Deserialize(renderElement, context);
        }
        Render = render;

        UpdateStatus("Settings loaded");
        Logger::Get().Log(MessageType::Info, "Project settings loaded");
    }

    void ApplyWorldSettingsToEngine()
    {
        const auto& world = World.Get();

        // Apply gravity
        /*
        SetGravity(world.gravity.x, world.gravity.y, world.gravity.z);
        */

        // Apply wind settings
        m_service.setWind(world.windType, world.windVector, world.windAmplitudes, world.windFrequencies);
        /*
        // Apply time settings
        SetTimeScale(world.timeScale);
        */

        Logger::Get().Log(MessageType::Info, "Applied world settings to engine");
    }

    void ExecuteCreatePrimitive()
    {
        PrimitiveMeshCreatedEvent event;
        event.primitive_type = PrimitiveType.Get();
        event.size = PrimitiveSize.Get();
        event.segments = PrimitiveSegments.Get();
        event.lod = PrimitiveLOD.Get();
        EventBus::Get().Publish(event);

        UpdateStatus("Created primitive");
    }

    void ExecuteLoadGeometry(const std::string& filepath)
    {
        // Load geometry file and add to active scene
        // Implementation details...
        UpdateStatus("Loaded geometry: " + fs::path(filepath).filename().string());
    }

    void UpdateStatus(const std::string& message)
    {
        StatusMessage = message;
    }
};