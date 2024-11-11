#include "Utils.h"
#include "imgui.h"
#include <iostream>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

bool Utils::s_showEnginePathPopup = false;

void Utils::SetEnvironmentVariable(const std::string& name, const std::string& value) {
    try {
#ifdef _WIN32 
        if (_putenv_s(name.c_str(), value.c_str()) == 0) return;
#else
        if (setenv(name.c_str(), value.c_str(), 1) == 0) return;
#endif
        std::cerr << "Failed to set environment variable: " << name << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error setting environment variable: " << e.what() << std::endl;
    }
}

std::string Utils::GetEnvironmentVariable(const std::string& name) {
    try {
        const char* value = std::getenv(name.c_str());
        if (value == nullptr) {
            std::cerr << "Environment variable not found: " << name << std::endl;
            return std::string();
        }
        return std::string(value);
    }
    catch (const std::exception& e) {
        std::cerr << "Error getting environment variable: " << e.what() << std::endl;
        return std::string();
    }
}

bool Utils::ShowSetEnginePathPopup() {
    static char pathBuffer[256] = "";
    bool pathSet = false;

    // Center the popup
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 center = viewport->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (s_showEnginePathPopup) {
        ImGui::OpenPopup("Set Engine Path");
    }

    if (ImGui::BeginPopupModal("Set Engine Path", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("DroneSim Engine path is not set!");
        ImGui::Text("Please enter the path to the DroneSim Engine directory:");
        ImGui::Spacing();

        ImGui::InputText("##Path", pathBuffer, sizeof(pathBuffer));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        bool pathValid = fs::exists(pathBuffer);
        if (!pathValid && pathBuffer[0] != '\0') {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Path does not exist!");
        }

        if (ImGui::Button("Set Path", ImVec2(120, 0))) {
            if (pathValid) {
                SetEnvironmentVariable("DRONESIM_ENGINE", pathBuffer);
                pathSet = true;
                s_showEnginePathPopup = false;
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            s_showEnginePathPopup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    return pathSet;
}