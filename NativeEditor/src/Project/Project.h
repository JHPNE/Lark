// Project.h
#pragma once
#include "ProjectTemplate.h"
#include <memory>
#include "../Utils/FileSystem.h"
#include "Scene.h"
#include <string>
#include <vector>
#include <tinyxml2.h>


class Project : public std::enable_shared_from_this<Project>, public ISerializable {
public:
    static constexpr const char* Extension = ".drosim";

    static std::shared_ptr<Project> Create(const std::string& name,
        const fs::path& path,
        const ProjectTemplate& tmpl);
    static std::shared_ptr<Project> Load(const fs::path& projectFile);

    bool SaveAs(const fs::path& newPath);
    void Unload();

    const std::string& GetName() const { return m_name; }
    const fs::path& GetPath() const { return m_path; }
    fs::path GetFullPath() const { return m_path / (m_name + Extension); }

    std::shared_ptr<Scene> AddScene(const std::string& sceneName);
    bool RemoveScene(uint32_t sceneId);
    std::shared_ptr<Scene> GetScene(uint32_t sceneId) const;
    std::shared_ptr<Scene> GetActiveScene() const { return m_activeScene; };
	bool SetActiveScene(uint32_t sceneId);
    const std::vector<std::shared_ptr<Scene>>& GetScenes() const { return m_scenes; }

    // Project modified state
    bool IsModified() const { return m_isModified; }
    void SetModified(bool modified = true) { m_isModified = modified; }

    // Serialization
    void Serialize(tinyxml2::XMLElement* element, SerializationContext& context) const override;
    bool Deserialize(const tinyxml2::XMLElement* element, SerializationContext& context) override;

    bool Save() {
        try {
            // Log current state
            Logger::Get().Log(MessageType::Info,
                "Saving project - Name: " + m_name +
                ", Path: " + m_path.string());

            tinyxml2::XMLDocument doc;
            SerializationContext context(doc);

            auto decl = doc.NewDeclaration();
            doc.LinkEndChild(decl);

            auto root = doc.NewElement("Project");
            doc.LinkEndChild(root);

            Serialize(root, context);

            auto fullPath = GetFullPath();
            Logger::Get().Log(MessageType::Info,
                "Saving to: " + fullPath.string());

            return doc.SaveFile(fullPath.string().c_str()) == tinyxml2::XML_SUCCESS;
        }
        catch (const std::exception& e) {
            Logger::Get().Log(MessageType::Error,
                "Error saving project: " + std::string(e.what()));
            return false;
        }
    }

private:
    Project(const std::string& name, const fs::path& path);

	// Internal Methods for Undo/Redo
    std::shared_ptr<Scene> AddSceneInternal(const std::string& sceneName);
    bool RemoveSceneInternal(uint32_t sceneId);
	uint32_t GenerateUniqueSceneID() const;
	std::shared_ptr<Scene> GetSceneById(uint32_t id) const;

    std::string m_name;
    fs::path m_path;
    std::vector<std::shared_ptr<Scene>> m_scenes;
	std::shared_ptr<Scene> m_activeScene;
    bool m_isModified = false;
};