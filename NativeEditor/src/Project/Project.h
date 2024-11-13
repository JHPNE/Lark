// Project.h
#pragma once
#include "ProjectTemplate.h"
#include <memory>
#include "../Utils/FileSystem.h" 
#include "../Utils/UndoRedo.h" 
#include "Scene.h"
#include <string>
#include <vector>
#include <tinyxml2.h>


class Project : public std::enable_shared_from_this<Project> {
public:
    static constexpr const char* Extension = ".drosim";

    static std::shared_ptr<Project> Create(const std::string& name,
        const fs::path& path,
        const ProjectTemplate& tmpl);
    static std::shared_ptr<Project> Load(const fs::path& projectFile);

    bool Save();
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

private:
    Project(const std::string& name, const fs::path& path);
    bool SaveScenesToXml(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* root) const;
    bool LoadScenesFromXml(tinyxml2::XMLElement* root);

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