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

void Project::Serialize(tinyxml2::XMLElement* element, SerializationContext& context) const {
    // Write project properties
    SerializerUtils::WriteAttribute(element, "version", "1.0");
    SerializerUtils::WriteElement(context.document, element, "Name", m_name);
    SerializerUtils::WriteElement(context.document, element, "Path", m_path.string());

    // Serialize scenes
    auto scenesElement = context.document.NewElement("Scenes");
    element->LinkEndChild(scenesElement);

    for (const auto& scene : m_scenes) {
        auto sceneElement = context.document.NewElement("Scene");
        SerializerUtils::WriteAttribute(sceneElement, "id", scene->GetID());

        if (scene == m_activeScene) {
            SerializerUtils::WriteAttribute(sceneElement, "active", true);
        }

        SerializerUtils::WriteElement(context.document, sceneElement, "Name", scene->GetName());

        // Serialize entities
        for (const auto& entity : scene->GetEntities()) {
            auto entityElement = context.document.NewElement("Entity");
            SerializerUtils::WriteAttribute(entityElement, "id", entity->GetID());
            SerializerUtils::WriteAttribute(entityElement, "name", entity->GetName());

            if (auto transform = entity->GetComponent<Transform>()) {
                auto transformElement = context.document.NewElement("Transform");
                transform->Serialize(transformElement, context);
                entityElement->LinkEndChild(transformElement);
            }

            if (auto script = entity->GetComponent<Script>()) {
                auto scriptElement = context.document.NewElement("Script");
                script->Serialize(scriptElement, context);
                entityElement->LinkEndChild(scriptElement);
            }

            if (auto geometry = entity->GetComponent<Geometry>()) {
                auto geometryElement = context.document.NewElement("Geometry");
                geometry->Serialize(geometryElement, context);
                entityElement->LinkEndChild(geometryElement);
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

    // Log the values we read
    Logger::Get().Log(MessageType::Info,
        "Read from XML - Name: " + name + ", Path: " + pathStr);

    // Update member variables
    m_name = name;
    m_path = fs::path(pathStr);

    Logger::Get().Log(MessageType::Info,
        "After assignment - Name: " + m_name +
        ", Path: " + m_path.string());

    // Read scenes
    auto scenesElement = element->FirstChildElement("Scenes");
    if (!scenesElement) return false;

    uint32_t activeSceneId = 0;

    for (auto sceneElement = scenesElement->FirstChildElement("Scene");
         sceneElement;
         sceneElement = sceneElement->NextSiblingElement("Scene")) {

        uint32_t id;
        std::string sceneName;
        bool isActive = false;

        if (!SerializerUtils::ReadAttribute(sceneElement, "id", id) ||
            !SerializerUtils::ReadElement(sceneElement, "Name", sceneName)) {
            continue;
        }

        SerializerUtils::ReadAttribute(sceneElement, "active", isActive);
        if (isActive) {
            activeSceneId = id;
        }

        auto scene = std::make_shared<Scene>(sceneName, id, shared_from_this());

        // Load entities
        for (auto entityElement = sceneElement->FirstChildElement("Entity");
             entityElement;
             entityElement = entityElement->NextSiblingElement("Entity")) {

            uint32_t entityId;
            std::string entityName;

            if (!SerializerUtils::ReadAttribute(entityElement, "id", entityId) ||
                !SerializerUtils::ReadAttribute(entityElement, "name", entityName)) {
                Logger::Get().Log(MessageType::Warning,
                    "Skipping entity with missing attributes in scene: " + scene->GetName());
                continue;
            }

            std::shared_ptr<GameEntity> entity = nullptr;
            if (auto geometryElement = entityElement->FirstChildElement("Geometry")) {
                auto geometryName = geometryElement->FirstChildElement("GeometryName")->Attribute("GeometryName");
                auto geometrySourceElement = geometryElement->FirstChildElement("GeometrySource")->Attribute("GeometrySourceElement");
                auto geometryType = geometryElement->FirstChildElement("GeometrySource")->Attribute("GeometryType");

                float size[3] = {5.0f, 5.0f, 5.0f};
                uint32_t segments[3] = {32, 16, 1};

                auto m_geometry = geometryType == "O"
                ? drosim::editor::Geometry::LoadGeometry(geometrySourceElement)
                : drosim::editor::Geometry::CreatePrimitive(
                        content_tools::PrimitiveMeshType::uv_sphere,
                        size,
                        segments
                    );

                auto geomType = geometryType == "0"
                ? GeometryType::ObjImport
                : GeometryType::PrimitiveType;

                uint32_t entityID = GeometryViewerView::Get().AddGeometry(geometryName, geometrySourceElement,  geomType, m_geometry.get());
                entity = scene->GetEntity(entityID);

            } else {
                entity = scene->CreateEntityInternal(entityName);
            }

            if (!entity) {
                Logger::Get().Log(MessageType::Error,
                    "Failed to create entity: " + entityName);
            }
            else {
                // Components who are not Bound to Entity Creation besides Transform
                if (auto transformElement = entityElement->FirstChildElement("Transform")) {
                    if (auto transform = entity->GetComponent<Transform>()) {
                        transform->Deserialize(transformElement, context);
                    }
                }

                //TODO use Deserializer
                if (auto scriptElement = entityElement->FirstChildElement("Script")) {
                    auto scriptName = scriptElement->FirstChildElement("ScriptName")->Attribute("Name");
                    auto it = std::find(m_loaded_scripts.begin(), m_loaded_scripts.end(), scriptName);
                    if (it != m_loaded_scripts.end()) {
                        ScriptInitializer scriptInit;
                        scriptInit.scriptName = scriptName;
                        scene->AddComponentToEntity<Script>(entity->GetID(), &scriptInit);
                    }
                }
            }
        }

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

