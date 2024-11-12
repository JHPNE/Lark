#pragma once
#include "Project.h"
#include "../Utils/Logger.h"
#include "../Utils/FileSystem.h"
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

		project->AddScene("Scene");

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

		if (!project->LoadScenesFromXml(root)) {
			Logger::Get().Log(MessageType::Warning, "Failed to load scenes from project file: " + projectFile.string());
		}

        // Set active Scene
		auto activeSceneElement = root->FirstChildElement("ActiveScene");
        if (activeSceneElement) {
			project->SetActiveScene(activeSceneElement->GetText());
        }
		else if (!project->GetScenes().empty()) {
			project->SetActiveScene(project->GetScenes().front()->GetName());
		}

		project->m_isModified = false;
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

		auto decl = doc.NewDeclaration();
		doc.LinkEndChild(decl);

		auto root = doc.NewElement("Project");
		doc.LinkEndChild(root);

        // Project properties
		auto nameElement = doc.NewElement("Name");
		nameElement->SetText(m_name.c_str());
		root->LinkEndChild(nameElement);

		auto pathElement = doc.NewElement("Path");
		pathElement->SetText(m_path.string().c_str());
		root->LinkEndChild(pathElement);

        // Active Scene
        if (m_activeScene) {
			auto activeSceneElement = doc.NewElement("ActiveScene");
			activeSceneElement->SetText(m_activeScene->GetName().c_str());
			root->LinkEndChild(activeSceneElement);
        }

        // Save scenes
        if (!SaveScenesToXml(doc, root)) {
            return false;
        }

        if (doc.SaveFile(GetFullPath().string().c_str()) != tinyxml2::XML_SUCCESS) {
            Logger::Get().Log(MessageType::Error, "Failed to save project file");
            return false;
        }

        m_isModified = false;
        Logger::Get().Log(MessageType::Info, "Project saved successfully");
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

std::shared_ptr<Scene> Project::AddScene(const std::string& sceneName) {
    auto scene = std::make_shared<Scene>(sceneName, shared_from_this());
    m_scenes.push_back(scene);

    if (!m_activeScene) {
        m_activeScene = scene;
    }

    SetModified();
    Logger::Get().Log(MessageType::Info, "Added scene: " + sceneName);
    return scene;
}

bool Project::RemoveScene(const std::string& sceneName) {
    auto it = std::find_if(m_scenes.begin(), m_scenes.end(),
        [&](const auto& scene) { return scene->GetName() == sceneName; });

    if (it != m_scenes.end()) {
        if (*it == m_activeScene) {
            m_activeScene = m_scenes.size() > 1 ? m_scenes.front() : nullptr;
        }
        m_scenes.erase(it);
        SetModified();
        Logger::Get().Log(MessageType::Info, "Removed scene: " + sceneName);
        return true;
    }
    return false;
}

bool Project::SetActiveScene(const std::string& sceneName) {
    auto scene = GetScene(sceneName);
    if (scene) {
        m_activeScene = scene;
        SetModified();
        return true;
    }
    return false;
}

std::shared_ptr<Scene> Project::GetScene(const std::string& sceneName) const {
    for (const auto& scene : m_scenes) {
        if (scene->GetName() == sceneName) {
            return scene;
        }
    }
    Logger::Get().Log(MessageType::Warning, "Scene not found: " + sceneName);
    return nullptr;
}

bool Project::SaveScenesToXml(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* root) const {
    auto scenesElement = doc.NewElement("Scenes");
    root->LinkEndChild(scenesElement);

    for (const auto& scene : m_scenes) {
        auto sceneElement = doc.NewElement("Scene");

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

    for (auto sceneElement = scenesElement->FirstChildElement("Scene");
        sceneElement;
        sceneElement = sceneElement->NextSiblingElement("Scene")) {

        auto nameElement = sceneElement->FirstChildElement("Name");
        if (!nameElement) continue;

        AddScene(nameElement->GetText());

        // TODO: Add game entity deserialization when implemented
    }
    return true;
}
