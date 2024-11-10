#pragma once
#include "Project.h"
#include "../Utils/Logger.h"
#include "../Utils/FileSystem.h"
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

        // Read and format project XML
        std::string projectXml = ReadFileContent(tmpl.GetProjectPath());
        if (projectXml.empty()) {
            Logger::Get().Log(MessageType::Error,
                "Failed to read template project file: " + tmpl.GetProjectPath().string());
            return nullptr;
        }

        std::string formattedXml = FormatProjectXml(projectXml, name, projectDir.string());

        // Platform-specific line endings
#ifdef _WIN32
        formattedXml += "\r\n";
#else
        formattedXml += "\n";
#endif

        std::ofstream projectFile(project->GetFullPath(), std::ios::binary);
        if (!projectFile || !(projectFile << formattedXml)) {
            Logger::Get().Log(MessageType::Error,
                "Failed to write project file: " + project->GetFullPath().string());
            return nullptr;
        }

        Logger::Get().Log(MessageType::Info, "Created project: " + name);
        return project;
    }
    catch (const fs::filesystem_error& e) {
        Logger::Get().Log(MessageType::Error,
            "Filesystem error while creating project: " + std::string(e.what()));
    }
    catch (const std::exception& e) {
        Logger::Get().Log(MessageType::Error,
            "General error while creating project: " + std::string(e.what()));
    }
    return nullptr;
}

std::shared_ptr<Project> Project::Load(const fs::path& projectFile) {
    try {
        if (!fs::exists(projectFile)) {
            Logger::Get().Log(MessageType::Error,
                "Project file does not exist: " + projectFile.string());
            return nullptr;
        }

        auto name = projectFile.stem().string();
        auto path = projectFile.parent_path();
        return std::shared_ptr<Project>(new Project(name, path));
    }
    catch (const fs::filesystem_error& e) {
        Logger::Get().Log(MessageType::Error,
            "Filesystem error while loading project: " + std::string(e.what()));
    }
    catch (const std::exception& e) {
        Logger::Get().Log(MessageType::Error,
            "General error while loading project: " + std::string(e.what()));
    }
    return nullptr;
}

bool Project::Save() {
    // Currently just returns true as the project file is created in Create()
    // Will be implemented when we add scene management
    return true;
}

void Project::Unload() {
    // Will be implemented when we add resource management
}

Project::Project(const std::string& name, const fs::path& path)
    : m_name(name)
    , m_path(path) {
}
