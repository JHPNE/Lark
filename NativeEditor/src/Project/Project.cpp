#pragma once
#include "Project.h"
#include "../Utils/Logger.h"
#include "../Utils/FileSystem.h"
#include "../Utils/GlobalUndoRedo.h"
#include <fstream>
#include "Scene.h"
#include "tinyxml2.h"

namespace {
    std::string ReadFileContent(const fs::path& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return "";
        return std::string(std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>());
    }

    std::string FormatProjectXml(const std::string& xml,
        const std::string& name,
        const std::string& path) {
        std::string result = xml;
        size_t pos;

        if ((pos = result.find("{0}")) != std::string::npos)
            result.replace(pos, 3, name);

        if ((pos = result.find("{1}")) != std::string::npos)
            result.replace(pos, 3, path);

        return result;
    }
}

std::shared_ptr<Project> Project::Create(const std::string& name,
    const fs::path& path, const ProjectTemplate& tmpl) {
    try {
        // Create project directory
        fs::path projectDir = path / name;
        fs::create_directories(projectDir);

        // Create project folders
        for (const auto& folder : tmpl.GetFolders()) {
            fs::create_directories(projectDir / folder);
        }

        // Create hidden directory
        fs::path hiddenDir = projectDir /
#ifdef _WIN32
            ".Drosim";
        // Set as hidden in Windows
        FileSystem::SetHidden(hiddenDir);
#else
            ".drosim";
        // On Unix-like systems, ensure directory has correct permissions if needed
        fs::permissions(hiddenDir, fs::perms::owner_all | fs::perms::group_all | fs::perms::others_read);
#endif
        fs::create_directories(hiddenDir);

        // Copy template files with error checking
        try {
            fs::copy_file(tmpl.GetIconPath(), hiddenDir / "Icon.png", fs::copy_options::overwrite_existing);
            fs::copy_file(tmpl.GetScreenshotPath(), hiddenDir / "Screenshot.png", fs::copy_options::overwrite_existing);
        }
        catch (const fs::filesystem_error& e) {
            Logger::Get().Log(MessageType::Error, "Failed to copy template files: " + std::string(e.what()));
            return nullptr;
        }

        // Create project instance
        auto project = std::shared_ptr<Project>(new Project(name, projectDir));

		project->AddSceneInternal("Scene");

        // Save initial project state
        if (!project->Save()) {
			Logger::Get().Log(MessageType::Error, "Failed to save project");
			return nullptr;
		}

        return project;
	}
    catch (const std::exception& e) {
        Logger::Get().Log(MessageType::Error,
            "General error while creating project: " + std::string(e.what()));
    }
    return nullptr;
}

std::shared_ptr<Project> Project::Load(const fs::path& projectFile) {
    try {
        tinyxml2::XMLDocument doc;
        if (doc.LoadFile(projectFile.string().c_str()) != tinyxml2::XML_SUCCESS) {
            Logger::Get().Log(MessageType::Error, "Failed to load project file: " + projectFile.string());
            return nullptr;
        }

        auto root = doc.FirstChildElement("Project");
        if (!root) {
            Logger::Get().Log(MessageType::Error, "Invalid project file " + projectFile.string());
            return nullptr;
        }

        auto nameElement = root->FirstChildElement("Name");
        auto pathElement = root->FirstChildElement("Path");

        if (!nameElement || !pathElement) {
            Logger::Get().Log(MessageType::Error, "Missing project properties! " + projectFile.string());
            return nullptr;
        }

        auto project = std::shared_ptr<Project>(new Project(nameElement->GetText(), pathElement->GetText()));

        // Load scenes - active scene will be set within LoadScenesFromXml
        if (!project->LoadScenesFromXml(root)) {
            Logger::Get().Log(MessageType::Warning, "Failed to load scenes from project file: " + projectFile.string());
            // Continue loading even if scenes fail - project might be empty
        }

        // Set unmodified since we just loaded
        project->m_isModified = false;

        // Log success
        Logger::Get().Log(MessageType::Info, "Successfully loaded project: " + project->GetName());

        return project;
    }
    catch (const std::exception& e) {
        Logger::Get().Log(MessageType::Error,
            "General error while loading project: " + std::string(e.what()));
    }
    return nullptr;
}

bool Project::Save() {
    try {
        tinyxml2::XMLDocument doc;

        // Add XML declaration
        auto decl = doc.NewDeclaration();
        doc.LinkEndChild(decl);

        // Create root element
        auto root = doc.NewElement("Project");
        doc.LinkEndChild(root);

        // Add version attribute for future compatibility
        root->SetAttribute("version", "1.0");

        // Project properties
        auto nameElement = doc.NewElement("Name");
        nameElement->SetText(m_name.c_str());
        root->LinkEndChild(nameElement);

        auto pathElement = doc.NewElement("Path");
        pathElement->SetText(m_path.string().c_str());
        root->LinkEndChild(pathElement);

        // Save scenes (active scene is handled within SaveScenesToXml)
        if (!SaveScenesToXml(doc, root)) {
            Logger::Get().Log(MessageType::Error, "Failed to save scenes");
            return false;
        }

        // Save to file
        if (doc.SaveFile(GetFullPath().string().c_str()) != tinyxml2::XML_SUCCESS) {
            Logger::Get().Log(MessageType::Error, "Failed to save project file: " + GetFullPath().string());
            return false;
        }

        m_isModified = false;
        Logger::Get().Log(MessageType::Info, "Project saved successfully: " + GetFullPath().string());
        return true;
    }
    catch (const std::exception& e) {
        Logger::Get().Log(MessageType::Error, "Error saving project: " + std::string(e.what()));
        return false;
    }
}

void Project::Unload() {
    // Will be implemented when we add resource management
}

Project::Project(const std::string& name, const fs::path& path)
    : m_name(name)
    , m_path(path)
    , m_activeScene(nullptr)
    , m_isModified(false) {
}

std::shared_ptr<Scene> Project::AddSceneInternal(const std::string& sceneName) {
    uint32_t newId = GenerateUniqueSceneID();
    auto scene = std::make_shared<Scene>(sceneName, newId, shared_from_this());
    m_scenes.push_back(scene);

    if (!m_activeScene) {
        m_activeScene = scene;
    }

    SetModified();
    Logger::Get().Log(MessageType::Info, "Added scene: " + sceneName);
    return scene;
}

bool Project::RemoveSceneInternal(uint32_t sceneId) {
    auto it = std::find_if(m_scenes.begin(), m_scenes.end(),
        [&](const auto& scene) { return scene->GetID() == sceneId; });

    if (it != m_scenes.end()) {
		std::string removedSceneName = (*it)->GetName();
        if (*it == m_activeScene) {
            m_activeScene = m_scenes.size() > 1 ? m_scenes.front() : nullptr;
        }
        it->get()->RemoveAllEntities();
        m_scenes.erase(it);
        SetModified();
        Logger::Get().Log(MessageType::Info, "Removed scene: " + removedSceneName);
        return true;
    }
    return false;
}

std::shared_ptr<Scene> Project::AddScene(const std::string& sceneName) {
    auto scene = AddSceneInternal(sceneName);

    if (scene) {
        // Store both ID and name since we can't lookup scene after deletion
        uint32_t sceneId = scene->GetID();
        std::string name = scene->GetName();

        auto action = std::make_shared<UndoRedoAction>(
            // Undo function - removes scene
            [this, sceneId]() {
                RemoveSceneInternal(sceneId);
            },
            // redo function - adds scene with same name
            [this, name]() {
                AddSceneInternal(name);
            },
            "Add Scene: " + name
        );


        GlobalUndoRedo::Instance().GetUndoRedo().Add(action);
    }

    return scene;
}

bool Project::RemoveScene(uint32_t sceneId) {
    auto sceneToRemove = GetSceneById(sceneId);
    if (!sceneToRemove) return false;

    // Store scene info before removal
    std::string sceneName = sceneToRemove->GetName();

    if (RemoveSceneInternal(sceneId)) {
        auto action = std::make_shared<UndoRedoAction>(
            // Undo function - recreates the scene
            [this, sceneName]() {
                AddSceneInternal(sceneName);
            },
            // Redo function - removes the scene again
            [this, sceneId]() {
                RemoveSceneInternal(sceneId);
            },
            "Remove Scene: " + sceneName
        );
        GlobalUndoRedo::Instance().GetUndoRedo().Add(action);
        return true;
    }
    return false;
}

bool Project::SetActiveScene(uint32_t sceneId) {
    auto scene = GetScene(sceneId);
    if (scene) {
        m_activeScene = scene;
        SetModified();
        return true;
    }
    return false;
}

std::shared_ptr<Scene> Project::GetScene(uint32_t sceneId) const {
	return GetSceneById(sceneId);
}

bool Project::SaveScenesToXml(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* root) const {
    auto scenesElement = doc.NewElement("Scenes");
    root->LinkEndChild(scenesElement);

    for (const auto& scene : m_scenes) {
        auto sceneElement = doc.NewElement("Scene");

		sceneElement->SetAttribute("ID", scene->GetID());

        if (scene == m_activeScene) {
			sceneElement->SetAttribute("Active", true);
        }

        auto nameElement = doc.NewElement("Name");
        nameElement->SetText(scene->GetName().c_str());
        sceneElement->LinkEndChild(nameElement);

        // TODO: Add game entity serialization when implemented

        scenesElement->LinkEndChild(sceneElement);
    }
    return true;
}

bool Project::LoadScenesFromXml(tinyxml2::XMLElement* root) {
    auto scenesElement = root->FirstChildElement("Scenes");
    if (!scenesElement) return false;

    uint32_t activeSceneId = 0;

    // First pass: load all scenes
    for (auto sceneElement = scenesElement->FirstChildElement("Scene");
        sceneElement;
        sceneElement = sceneElement->NextSiblingElement("Scene")) {

        // Get scene ID from attribute
        uint32_t id;
        if (sceneElement->QueryUnsignedAttribute("id", &id) != tinyxml2::XML_SUCCESS) {
            continue;  // Skip if no valid ID
        }

        // Check if this is the active scene
        bool isActive = false;
        sceneElement->QueryBoolAttribute("active", &isActive);
        if (isActive) {
            activeSceneId = id;
        }

        auto nameElement = sceneElement->FirstChildElement("Name");
        if (!nameElement) continue;

        auto scene = std::make_shared<Scene>(
            nameElement->GetText(),
            id,
            shared_from_this()
        );

        m_scenes.push_back(scene);
    }

    // Set active scene
    if (activeSceneId != 0) {
        SetActiveScene(activeSceneId);
    }
    else if (!m_scenes.empty()) {
        SetActiveScene(m_scenes.front()->GetID());
    }

    return true;
}

uint32_t Project::GenerateUniqueSceneID() const {
    // If there are no scenes, start with ID 1
    if (m_scenes.empty()) {
        return 1;
    }

    // Otherwise find highest existing ID and increment by 1
    uint32_t maxId = 0;  // Start from 0 instead of m_nextSceneId
    for (const auto& scene : m_scenes) {
        maxId = std::max(maxId, scene->GetID());
    }
    return maxId + 1;
}

std::shared_ptr<Scene> Project::GetSceneById(uint32_t id) const {
    auto it = std::find_if(m_scenes.begin(), m_scenes.end(),
        [id](const auto& scene) { return scene->GetID() == id; });

    if (it != m_scenes.end()) {
        return *it;
    }

    return nullptr;
}

