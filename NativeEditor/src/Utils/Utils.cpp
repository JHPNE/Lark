#include "Utils.h"
#include "imgui.h"
#include <iostream>
#include <filesystem>
#include <string>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <pwd.h>
#include <unistd.h>
#endif

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

fs::path Utils::GetApplicationDataPath() {
    try {
#ifdef __APPLE__
        // Get user's home directory
        const char* homeDir = getenv("HOME");
        if (!homeDir) {
            struct passwd* pw = getpwuid(getuid());
            if (pw) homeDir = pw->pw_dir;
        }
        if (!homeDir) {
            throw std::runtime_error("Could not determine home directory");
        }
        return fs::path(homeDir) / "Library/Application Support/DrosimEditor";
#else
        return fs::path(GetEnvironmentVariable("APPDATA")) / "DrosimEditor";
#endif
    }
    catch (const std::exception& e) {
        std::cerr << "Error getting application data path: " << e.what() << std::endl;
        return fs::path();
    }
}

std::optional<fs::path> Utils::GetBundlePath() {
#ifdef __APPLE__
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    if (!mainBundle) return std::nullopt;

    CFURLRef bundleURL = CFBundleCopyBundleURL(mainBundle);
    if (!bundleURL) return std::nullopt;

    char path[PATH_MAX];
    if (!CFURLGetFileSystemRepresentation(bundleURL, true, (UInt8*)path, PATH_MAX)) {
        CFRelease(bundleURL);
        return std::nullopt;
    }

    CFRelease(bundleURL);
    return fs::path(path);
#else
    return std::nullopt;
#endif
}

fs::path Utils::GetEngineResourcePath() {
    std::string enginePathString = GetEnvironmentVariable("DRONESIM_ENGINE");

    if (enginePathString.empty()) {
        enginePathString = GetDefaultEnginePath().string();
    }

#ifdef __APPLE__
    auto bundlePath = GetBundlePath();
    if (bundlePath) {
        return *bundlePath / "Contents/Resources/ProjectTemplates";
    }
    // Fallback to engine path
    return fs::path(enginePathString) / "NativeEditor/ProjectTemplates";
#else
    return fs::path(enginePathString) / "NativeEditor/ProjectTemplates";
#endif
}

fs::path Utils::GetDefaultEnginePath() {
#ifdef __APPLE__
    return fs::path("/Applications/DroneSim");
#else
    return fs::path("C:/Program Files/DroneSim");
#endif
}

bool Utils::ShowSetEnginePathPopup() {
    static char pathBuffer[256] = "";
    static bool initialized = false;
    bool pathSet = false;

    if (!initialized) {
        auto defaultPath = GetDefaultEnginePath().string();
        strncpy(pathBuffer, defaultPath.c_str(), sizeof(pathBuffer) - 1);
        initialized = true;
    }

    // Center the popup
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 center = viewport->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (s_showEnginePathPopup) {
        ImGui::OpenPopup("Set Engine Path");
    }

    // Handle DPI scaling for Mac
#ifdef __APPLE__
    float dpiScale = 1.0f;
    if (auto window = static_cast<GLFWwindow*>(ImGui::GetMainViewport()->PlatformHandle)) {
        int fbWidth, fbHeight, winWidth, winHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        glfwGetWindowSize(window, &winWidth, &winHeight);
        dpiScale = static_cast<float>(fbWidth) / winWidth;
    }
    ImGui::SetNextWindowSize(ImVec2(400 * dpiScale, 0));
#endif

    if (ImGui::BeginPopupModal("Set Engine Path", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("DroneSim Engine path is not set!");
        ImGui::Text("Please enter the path to the DroneSim Engine directory:");
        ImGui::Spacing();

        ImGui::InputText("##Path", pathBuffer, sizeof(pathBuffer));

        if (ImGui::Button("Use Default Path")) {
            auto defaultPath = GetDefaultEnginePath().string();
            strncpy(pathBuffer, defaultPath.c_str(), sizeof(pathBuffer) - 1);
        }

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