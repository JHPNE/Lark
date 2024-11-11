#include "Scene.h"
#include "Project.h"
#include "../Utils/Logger.h"

Scene::Scene(const std::string& name, Project* project)
    : m_name(name)
    , m_project(project)
    , m_isActive(false) {
}

std::shared_ptr<Scene> Scene::Create(const std::string& name, Project* project) {
    try {
        if (!project) {
            Logger::Get().Log(MessageType::Error, "Cannot create scene: Invalid project pointer");
            return nullptr;
        }

        if (name.empty()) {
            Logger::Get().Log(MessageType::Error, "Cannot create scene: Empty name");
            return nullptr;
        }

        auto scene = std::shared_ptr<Scene>(new Scene(name, project));
        Logger::Get().Log(MessageType::Info, "Created scene: " + name);
        return scene;
    }
    catch (const std::exception& e) {
        Logger::Get().Log(MessageType::Error,
            "General error while creating scene: " + std::string(e.what()));
    }
    return nullptr;
}