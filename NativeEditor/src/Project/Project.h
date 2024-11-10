// Project.h
#pragma once
#include "ProjectTemplate.h"
#include <memory>
#include "../Utils/FileSystem.h" 
#include <string>
#include <vector>


class Project {
public:
    static constexpr const char* Extension = ".drosim";

    static std::shared_ptr<Project> Create(const std::string& name,
        const fs::path& path,
        const ProjectTemplate& tmpl);
    static std::shared_ptr<Project> Load(const fs::path& projectFile);

    bool Save();
    void Unload();

    const std::string& GetName() const { return m_name; }
    const fs::path& GetPath() const { return m_path; }
    fs::path GetFullPath() const { return m_path / (m_name + Extension); }

private:
    Project(const std::string& name, const fs::path& path);

    std::string m_name;
    fs::path m_path;
};