#pragma once
#include <string>
#include <memory>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

class ProjectTemplate {
public:
    static std::vector<std::shared_ptr<ProjectTemplate>> LoadTemplates(const fs::path& templatePath);
    static std::shared_ptr<ProjectTemplate> LoadFromFile(const fs::path& templateFile);

    static constexpr const char* templatePath = R"(..\..\DrosimEditor\ProjectTemplates)";

    const std::string& GetType() const { return m_type; }
    const std::string& GetProjectFile() const { return m_file; }
    const std::vector<std::string>& GetFolders() const { return m_folders; }
    const fs::path& GetIconPath() const { return m_iconPath; }
    const fs::path& GetScreenshotPath() const { return m_screenshotPath; }
    const fs::path& GetProjectPath() const { return m_projectPath; }
    const fs::path& GetTemplatePath() const { return m_templatePath; }

    void SetType(const std::string& type) { m_type = type; }
    void SetProjectFile(const std::string& file) { m_file = file; }
    void AddFolder(const std::string& folder) { m_folders.push_back(folder); }
private:
    std::string m_type;
    std::string m_file;
    std::vector<std::string> m_folders;
    fs::path m_iconPath;
    fs::path m_screenshotPath;
    fs::path m_projectPath;
    fs::path m_templatePath;
};