#include "Project.h"
#include "../Utils/Etc/FileSystem.h"
#include "../Utils/Etc/Logger.h"
#include "../Utils/System/GlobalUndoRedo.h"
#include "EngineAPI.h"
#include "Geometry/Geometry.h"
#include "Scene.h"
#include "View/GeometryViewerView.h"
#include "tinyxml2.h"

#include <fstream>

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
            ".lark";
        // Set as hidden in Windows
        FileSystem::SetHidden(hiddenDir);
#else
            ".lark";
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

        SerializationContext context(doc);
        auto project = std::shared_ptr<Project>(new Project("", ""));
        project->LoadScripts(projectFile);

        if (!project->Deserialize(root, context)) {
            Logger::Get().Log(MessageType::Error, "Failed to deserialize project: " + projectFile.string());
            return nullptr;
        }

        project->m_isModified = false;
        Logger::Get().Log(MessageType::Info, "Successfully loaded project: " + project->GetName());

        return project;
    }
    catch (const std::exception& e) {
        Logger::Get().Log(MessageType::Error,
            "General error while loading project: " + std::string(e.what()));
        return nullptr;
    }
}


void Project::Unload() {
    for (auto& scene : m_scenes) {
        for (auto& entity : scene->GetEntities()) {
            RemoveGameEntity(entity->GetID());
        }
        scene->RemoveAllEntities();
    }


    m_scenes.clear();
    m_activeScene = nullptr;

    GlobalUndoRedo::Instance().GetUndoRedo().Reset();

    m_isModified = false;

    Logger::Get().Log(MessageType::Info, "Successfully unloaded project: " + GetName());
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

void Project::Serialize(tinyxml2::XMLElement* element, SerializationContext& context) const {
    WriteVersion(element);
    SerializerUtils::WriteElement(context.document, element, "Name", m_name);
    SerializerUtils::WriteElement(context.document, element, "Path", m_path.string());

    // Serialize scenes
    auto scenesElement = context.document.NewElement("Scenes");
    element->LinkEndChild(scenesElement);

    for (const auto& scene : m_scenes) {
        auto sceneElement = context.document.NewElement("Scene");
        SerializerUtils::WriteAttribute(sceneElement, "id", scene->GetID());
        SerializerUtils::WriteAttribute(sceneElement, "active", scene == m_activeScene);
        SerializerUtils::WriteElement(context.document, sceneElement, "Name", scene->GetName());

        for (const auto& entity : scene->GetEntities()) {
            auto entityElement = context.document.NewElement("Entity");
            SerializerUtils::WriteAttribute(entityElement, "id", entity->GetID());
            SerializerUtils::WriteAttribute(entityElement, "name", entity->GetName());

            for (const auto& [compType, compPtr] : entity->GetAllComponents()) {
                if (auto serializable = dynamic_cast<const ISerializable*>(compPtr.get())) {
                    auto compName = Component::ComponentTypeToString(compType); // Implement this function
                    auto compElement = context.document.NewElement(compName);
                    serializable->Serialize(compElement, context);
                    entityElement->LinkEndChild(compElement);
                }
            }
            sceneElement->LinkEndChild(entityElement);
        }
        scenesElement->LinkEndChild(sceneElement);
    }
}

bool Project::Deserialize(const tinyxml2::XMLElement* element, SerializationContext& context) {
    // Read project properties
    std::string name, pathStr;
    if (!SerializerUtils::ReadElement(element, "Name", name) ||
        !SerializerUtils::ReadElement(element, "Path", pathStr)) {
        Logger::Get().Log(MessageType::Error, "Failed to read Name or Path elements");
        return false;
        }

    // Update member variables
    m_name = name;
    m_path = fs::path(pathStr);

    // Read scenes
    auto scenesElement = element->FirstChildElement("Scenes");
    if (!scenesElement) return false;

    uint32_t activeSceneId = 0;

    for (auto sceneElement = scenesElement->FirstChildElement("Scene");
         sceneElement; sceneElement = sceneElement->NextSiblingElement("Scene")) {

        uint32_t id; std::string sceneName; bool active = false;

        SerializerUtils::ReadAttribute(sceneElement, "id", id);
        SerializerUtils::ReadAttribute(sceneElement, "active", active);
        SerializerUtils::ReadElement(sceneElement, "Name", sceneName);

        auto scene = std::make_shared<Scene>(sceneName, id, shared_from_this());
        if (active) m_activeScene = scene;

        // Load entities
        for (auto entityElement = sceneElement->FirstChildElement("Entity");
            entityElement; entityElement = entityElement->NextSiblingElement("Entity")) {
            uint32_t entityId; std::string entityName;

            SerializerUtils::ReadAttribute(entityElement, "id", entityId);
            SerializerUtils::ReadAttribute(entityElement, "name", entityName);

            const std::shared_ptr<GameEntity> entity = scene->CreateEntityInternal(entityName);
            if (!entity) continue;

            for (auto *compElement = entityElement->FirstChildElement();
                compElement; compElement = compElement->NextSiblingElement()) {
                const std::string compName = compElement->Value();

                if (compName == "Transform") {
                    if (const auto transform = entity->GetComponent<Transform>()) {
                        transform->Deserialize(compElement, context);
                    }
                }
                else if (compName == "Script") {
                    HandleScriptDeserialization(compElement, entity, context);
                }
                else if (compName == "Geometry") {
                    HandleGeometryDeserialization(compElement, entity, context);
                }
            }

            // Now create the engine entity with all components
            scene->FinalizeEntityCreation(entity);
        }
        m_scenes.push_back(scene);
    }

    // Set active scene if none was marked as active
    if (!m_activeScene && !m_scenes.empty()) {
        m_activeScene = m_scenes.front();
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

bool Project::LoadScripts(fs::path projectFile) {
    auto scriptDir = projectFile.parent_path() / "SimCode";
    if (!fs::exists(scriptDir)) return false;

    fs::path scriptPath(scriptDir);
    for (auto& files : fs::directory_iterator(scriptPath)) {
        auto file = files.path();
        if (file.extension() == ".py") {
            std::string scriptName = file.filename().replace_extension("").string();

            m_loaded_scripts.emplace_back(scriptName);
            RegisterScript(scriptName.c_str());
        }
    }

    return true;
}

bool Project::CreateNewScript(const char *scriptName) {
    auto scriptDir = m_path / "SimCode" ;
    if (!fs::exists(scriptDir)) return false;

    fs::path scriptPath = scriptDir / (std::string(scriptName) + ".py");
    std::ofstream scriptFile(scriptPath);
    if (scriptFile.is_open()) {
        // Write template Python script content
        scriptFile << "class "<< scriptName <<":\n"
                  << "    def __init__(self, entity):\n"
                  << "        self.entity = entity\n\n"
                  << "    def begin_play(self):\n"
                  << "        # Initialize script here\n"
                  << "        pass\n\n"
                  << "    def update(self, delta_time):\n"
                  << "        # Update logic here\n"
                  << "        pass\n";

        scriptFile.close();

        // Register script through EngineAPI
        if (RegisterScript(scriptName)) {
            Logger::Get().Log(MessageType::Info, "Created and registered script: " + scriptPath.string());
        } else {
            Logger::Get().Log(MessageType::Error, "Failed to register script: " + scriptPath.string());
            return false;
        }
    } else {
        Logger::Get().Log(MessageType::Error, "Failed to create script: " + scriptPath.string());
        return false;
    }

    return true;
}

