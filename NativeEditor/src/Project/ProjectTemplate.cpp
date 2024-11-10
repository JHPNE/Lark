#pragma once
#include "ProjectTemplate.h"
#include "../Utils/Logger.h"
#include <fstream>

namespace {
    std::string ReadFileContent(const fs::path& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return "";
        return std::string(std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>());
    }

    bool ParseTemplateXml(const std::string& xml, ProjectTemplate& tmpl) {
        // Simple parse for these tags
        const std::string typeTag = "<pType>";
        const std::string typeEndTag = "</pType>";
        const std::string fileTag = "<pFile>";
        const std::string fileEndTag = "</pFile>";
        const std::string folderStartTag = "<a:string>";
        const std::string folderEndTag = "</a:string>";

        // Get type
        size_t typeStart = xml.find(typeTag);
        size_t typeEnd = xml.find(typeEndTag);
        if (typeStart != std::string::npos && typeEnd != std::string::npos) {
            typeStart += typeTag.length();
            tmpl.SetType(xml.substr(typeStart, typeEnd - typeStart));
        }

        // Get project file
        size_t fileStart = xml.find(fileTag);
        size_t fileEnd = xml.find(fileEndTag);
        if (fileStart != std::string::npos && fileEnd != std::string::npos) {
            fileStart += fileTag.length();
            tmpl.SetProjectFile(xml.substr(fileStart, fileEnd - fileStart));
        }

        // Get folders
        size_t folderPos = 0;
        while ((folderPos = xml.find(folderStartTag, folderPos)) != std::string::npos) {
            folderPos += folderStartTag.length();
            size_t endPos = xml.find(folderEndTag, folderPos);
            if (endPos != std::string::npos) {
                tmpl.AddFolder(xml.substr(folderPos, endPos - folderPos));
                folderPos = endPos + folderEndTag.length();
            }
        }

        return !tmpl.GetType().empty() && !tmpl.GetProjectFile().empty();
    }
}

std::vector<std::shared_ptr<ProjectTemplate>> ProjectTemplate::LoadTemplates(const fs::path& templatePath) {
    std::vector<std::shared_ptr<ProjectTemplate>> templates;

    try {
        for (const auto& entry : fs::recursive_directory_iterator(templatePath)) {
            if (entry.is_regular_file() && entry.path().filename() == "template.xml") {
                if (auto tmpl = LoadFromFile(entry.path())) {
                    templates.push_back(tmpl);
                }
            }
        }
    }
    catch (const std::exception& e) {
        Logger::Get().Log(MessageType::Error,
            "Failed to load project templates: " + std::string(e.what()));
    }

    return templates;
}

std::shared_ptr<ProjectTemplate> ProjectTemplate::LoadFromFile(const fs::path& templateFile) {
    try {
        std::string content = ReadFileContent(templateFile);
        if (content.empty()) {
            Logger::Get().Log(MessageType::Error,
                "Failed to read template file: " + templateFile.string());
            return nullptr;
        }

        auto tmpl = std::make_shared<ProjectTemplate>();
        if (!ParseTemplateXml(content, *tmpl)) {
            Logger::Get().Log(MessageType::Error,
                "Failed to parse template file: " + templateFile.string());
            return nullptr;
        }

        // Set up paths
        tmpl->m_templatePath = templateFile.parent_path();
        tmpl->m_iconPath = tmpl->m_templatePath / "Icon.png";
        tmpl->m_screenshotPath = tmpl->m_templatePath / "Screenshot.png";
        tmpl->m_projectPath = tmpl->m_templatePath / tmpl->m_file;

        // Verify required files exist
        if (!fs::exists(tmpl->m_iconPath) ||
            !fs::exists(tmpl->m_screenshotPath) ||
            !fs::exists(tmpl->m_projectPath)) {
            Logger::Get().Log(MessageType::Error,
                "Missing required template files in: " + templateFile.string());
            return nullptr;
        }

        return tmpl;
    }
    catch (const std::exception& e) {
        Logger::Get().Log(MessageType::Error,
            "Failed to load template: " + std::string(e.what()));
        return nullptr;
    }
	return nullptr;
}
