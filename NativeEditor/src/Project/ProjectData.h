#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "Project.h"


namespace fs = std::filesystem;

struct ProjectData {
	std::string name;
	fs::path path;
	std::string date;
	
	fs::path GetFullPath() const {
		return path / (name + Project::Extension);
	}

	// contains Projects and Path to Project which contains the Info
    static bool ParseProjectXml(const std::string& xml, std::vector<ProjectData>& outProjects) {
        const std::string projectDataTag = "<ProjectData>";
        const std::string projectDataEndTag = "</ProjectData>";
        const std::string dateTag = "<Date>";
        const std::string dateEndTag = "</Date>";
        const std::string nameTag = "<ProjectName>";
        const std::string nameEndTag = "</ProjectName>";
        const std::string pathTag = "<ProjectPath>";
        const std::string pathEndTag = "</ProjectPath>";

        size_t currentPos = 0;
        while ((currentPos = xml.find(projectDataTag, currentPos)) != std::string::npos) {
            ProjectData projectData;
            size_t endPos = xml.find(projectDataEndTag, currentPos);
            if (endPos == std::string::npos) break;

            // Get project segment
            std::string projectSegment = xml.substr(currentPos, endPos - currentPos + projectDataEndTag.length());

            // Parse date
            size_t dateStart = projectSegment.find(dateTag);
            if (dateStart != std::string::npos) {
                dateStart += dateTag.length();
                size_t dateEnd = projectSegment.find(dateEndTag, dateStart);
                projectData.date = projectSegment.substr(dateStart, dateEnd - dateStart);
            }

            // Parse name
            size_t nameStart = projectSegment.find(nameTag);
            if (nameStart != std::string::npos) {
                nameStart += nameTag.length();
                size_t nameEnd = projectSegment.find(nameEndTag, nameStart);
                projectData.name = projectSegment.substr(nameStart, nameEnd - nameStart);
            }

            // Parse path
            size_t pathStart = projectSegment.find(pathTag);
            if (pathStart != std::string::npos) {
                pathStart += pathTag.length();
                size_t pathEnd = projectSegment.find(pathEndTag, pathStart);
                projectData.path = projectSegment.substr(pathStart, pathEnd - pathStart);
            }

            if (!projectData.name.empty() && !projectData.path.empty()) {
                outProjects.push_back(projectData);
            }

            currentPos = endPos + projectDataEndTag.length();
        }

        return !outProjects.empty();
    }
};