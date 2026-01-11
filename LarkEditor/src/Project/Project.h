// Project.h
#pragma once
#include "../Utils/Etc/FileSystem.h"
#include "ProjectTemplate.h"
#include "Scene.h"
#include <memory>
#include <string>
#include <tinyxml2.h>
#include <vector>

class Project : public std::enable_shared_from_this<Project>, public ISerializable
{
  public:
    static constexpr const char *Extension = ".lark";

    static std::shared_ptr<Project> Create(const std::string &name, const fs::path &path,
                                           const ProjectTemplate &tmpl);
    static std::shared_ptr<Project> Load(const fs::path &projectFile);

    bool SaveAs(const fs::path &newPath);
    void Unload();

    const std::string &GetName() const { return m_name; }
    const fs::path &GetPath() const { return m_path; }
    fs::path GetFullPath() const { return m_path / (m_name + Extension); }

    std::shared_ptr<Scene> AddScene(const std::string &sceneName);
    bool RemoveScene(uint32_t sceneId);
    std::shared_ptr<Scene> GetScene(uint32_t sceneId) const;
    std::shared_ptr<Scene> GetActiveScene() const { return m_activeScene; };
    bool SetActiveScene(uint32_t sceneId);
    const std::vector<std::shared_ptr<Scene>> &GetScenes() const { return m_scenes; }

    // Project modified state
    bool IsModified() const { return m_isModified; }
    void SetModified(bool modified = true) { m_isModified = modified; }

    // Serialization
    void Serialize(tinyxml2::XMLElement *element, SerializationContext &context) const override;
    bool Deserialize(const tinyxml2::XMLElement *element, SerializationContext &context) override;

    // Script helper
    bool CreateNewScript(const char *scriptName);
    bool LoadScripts(fs::path projectFile);

    bool Save()
    {
        try
        {
            tinyxml2::XMLDocument doc;
            SerializationContext context(doc);

            auto decl = doc.NewDeclaration();
            doc.LinkEndChild(decl);

            auto root = doc.NewElement("Project");
            doc.LinkEndChild(root);

            Serialize(root, context);

            if (context.HasErrors())
            {
                for (const auto &error : context.errors)
                {
                    Logger::Get().Log(MessageType::Error, "Serialization Error: " + error);
                }
                return false;
            }

            for (const auto &warning : context.warnings)
            {
                Logger::Get().Log(MessageType::Warning, "Serialization Warning: " + warning);
            }
            auto fullPath = GetFullPath();
            Logger::Get().Log(MessageType::Info, "Saving to: " + fullPath.string());

            bool success = doc.SaveFile(fullPath.string().c_str()) == tinyxml2::XML_SUCCESS;
            if (success)
            {
                m_isModified = false;
                Logger::Get().Log(MessageType::Info, "Project saved succesfully");
            }
            else
            {
                Logger::Get().Log(MessageType::Error, "Failed to save project");
            }

            return success;
        }
        catch (const std::exception &e)
        {
            Logger::Get().Log(MessageType::Error, "Error saving project: " + std::string(e.what()));
            return false;
        }
    }

  private:
    Project(const std::string &name, const fs::path &path);

    void HandleScriptDeserialization(const tinyxml2::XMLElement *compElement,
                                     const std::shared_ptr<GameEntity> &entity,
                                     SerializationContext &context)
    {
        ScriptInitializer scriptInit;
        if (auto scriptNameElement = compElement->FirstChildElement("ScriptName"))
        {
            const char *name = scriptNameElement->Attribute("Name");
            if (name)
            {
                scriptInit.scriptName = name;
                // Check if script is loaded
                auto it = std::find(m_loaded_scripts.begin(), m_loaded_scripts.end(),
                                    scriptInit.scriptName);
                if (it != m_loaded_scripts.end())
                {
                    auto *script = entity->AddComponent<Script>(&scriptInit);
                    if (script)
                    {
                        script->Deserialize(compElement, context);
                    }
                }
            }
        }
    }

    void HandleGeometryDeserialization(const tinyxml2::XMLElement *compElement,
                                       const std::shared_ptr<GameEntity> &entity,
                                       SerializationContext &context)
    {
        GeometryInitializer geometryInit;

        auto *geometry = entity->AddComponent<Geometry>(&geometryInit);
        if (geometry)
        {
            geometry->Deserialize(compElement, context);
        }
    }

    void HandlePhysicDeserialization(const tinyxml2::XMLElement * compElement,
                                     const std::shared_ptr<GameEntity> &entity,
                                     SerializationContext &context)
    {
        PhysicInitializer physicInit;

        auto *physic = entity->AddComponent<Physics>(&physicInit);
        if (physic)
        {
            physic->Deserialize(compElement, context);
        }
    }

    void HandleMaterialDeserialization(const tinyxml2::XMLElement* compElement,
                                     const std::shared_ptr<GameEntity> &entity,
                                     SerializationContext &context)
    {
        MaterialInitializer materialInit;

        auto *material = entity->AddComponent<Material>(&materialInit);
        if (material)
        {
            material->Deserialize(compElement, context);
        }
    }

    // Internal Methods for Undo/Redo
    std::shared_ptr<Scene> AddSceneInternal(const std::string &sceneName);
    bool RemoveSceneInternal(uint32_t sceneId);
    uint32_t GenerateUniqueSceneID() const;
    std::shared_ptr<Scene> GetSceneById(uint32_t id) const;

    std::string m_name;
    fs::path m_path;
    std::vector<std::shared_ptr<Scene>> m_scenes;
    std::shared_ptr<Scene> m_activeScene;
    std::vector<std::string> m_loaded_scripts;
    bool m_isModified = false;
};