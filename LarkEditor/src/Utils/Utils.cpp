#include "Utils.h"
#include "imgui.h"
#include <iostream>
#include <filesystem>
#include <string>
#include "GLFW/glfw3.h"

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <pwd.h>
#include <unistd.h>
#endif

bool Utils::s_showEnginePathPopup = false;

void Utils::SetEnvVar(const std::string& name, const std::string& value) {
    try {
#ifdef _WIN32
        // Set for current process
        if (_putenv_s(name.c_str(), value.c_str()) != 0) {
            std::cerr << "Failed to set environment variable for current process: " << name << std::endl;
            return;
        }

        // Set persistently in Windows Registry for user
        HKEY hKey;
        LONG result = RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_WRITE, &hKey);

        if (result == ERROR_SUCCESS) {
            result = RegSetValueExA(hKey, name.c_str(), 0, REG_SZ,
                                   (const BYTE*)value.c_str(),
                                   (DWORD)(value.length() + 1));
            RegCloseKey(hKey);

            if (result == ERROR_SUCCESS) {
                // Notify the system about the environment change
                SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                                   (LPARAM)"Environment", SMTO_ABORTIFHUNG,
                                   5000, nullptr);

                std::cout << "Successfully set persistent environment variable: " << name << std::endl;
            } else {
                std::cerr << "Failed to set registry value: " << name << std::endl;
            }
        } else {
            std::cerr << "Failed to open registry key" << std::endl;
        }
#else
        // For Unix-like systems
        if (setenv(name.c_str(), value.c_str(), 1) == 0) {
            // Also try to save to shell profile for persistence
            SaveToShellProfile(name, value);
        } else {
            std::cerr << "Failed to set environment variable: " << name << std::endl;
        }
#endif
    }
    catch (const std::exception& e) {
        std::cerr << "Error setting environment variable: " << e.what() << std::endl;
    }
}


std::string Utils::GetEnvVar(const std::string& name) {
    try {
        const char* value = std::getenv(name.c_str());
        if (value == nullptr) {
            // Try to load from persistent storage if not in current environment
#ifdef _WIN32
            char buffer[1024];
            DWORD bufferSize = sizeof(buffer);
            HKEY hKey;

            if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                if (RegQueryValueExA(hKey, name.c_str(), nullptr, nullptr,
                                    (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
                    RegCloseKey(hKey);
                    // Set it in current process too
                    _putenv_s(name.c_str(), buffer);
                    return std::string(buffer);
                                    }
                RegCloseKey(hKey);
            }
#endif

            // If still not found, check if we should auto-setup
            if (name == "LARK_ENGINE" && ShouldAutoSetup()) {
                auto defaultPath = GetDefaultEnginePath();
                if (fs::exists(defaultPath) && fs::exists(defaultPath / "LarkEditor/ProjectTemplates")) {
                    SetEnvVar(name, defaultPath.string());
                    return defaultPath.string();
                } else {
                    // Show popup to set path
                    s_showEnginePathPopup = true;
                }
            }

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

bool Utils::ShouldAutoSetup() {
    // Check if config file exists that indicates we should auto-setup
    auto configPath = GetApplicationDataPath() / "config.ini";
    if (!fs::exists(configPath)) {
        // First run - create config and return true
        try {

        } catch (...) {}
        return true;
    }
    return false;
}

#ifndef _WIN32
void Utils::SaveToShellProfile(const std::string& name, const std::string& value) {
    // Try to save to common shell profiles
    const char* home = getenv("HOME");
    if (!home) return;

    std::vector<std::string> profiles = {
        std::string(home) + "/.bashrc",
        std::string(home) + "/.zshrc",
        std::string(home) + "/.profile"
    };

    for (const auto& profile : profiles) {
        if (fs::exists(profile)) {
            std::ofstream file(profile, std::ios::app);
            if (file.is_open()) {
                file << "\n# Added by Lark\n";
                file << "export " << name << "=\"" << value << "\"\n";
                file.close();
            }
        }
    }
}
#endif

fs::path Utils::GetApplicationDataPath() {
    try {
#ifdef _WIN32
        char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path))) {
            return fs::path(path) / "DrosimEditor";
        }
        // Fallback
        const char* appdata = std::getenv("APPDATA");
        if (appdata) {
            return fs::path(appdata) / "DrosimEditor";
        }
        return fs::path("C:/ProgramData/DrosimEditor");
#elif defined(__APPLE__)
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
        const char* homeDir = getenv("HOME");
        if (!homeDir) {
            struct passwd* pw = getpwuid(getuid());
            if (pw) homeDir = pw->pw_dir;
        }
        if (!homeDir) {
            throw std::runtime_error("Could not determine home directory");
        }
        return fs::path(homeDir) / ".config/DrosimEditor";
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
    std::string enginePathString = GetEnvVar("LARK_ENGINE");

    if (enginePathString.empty()) {
        // Try to find the engine path automatically
        auto defaultPath = GetDefaultEnginePath();
        if (fs::exists(defaultPath / "LarkEditor/ProjectTemplates")) {
            SetEnvVar("LARK_ENGINE", defaultPath.string());
            enginePathString = defaultPath.string();
        } else {
            // Return empty path to trigger popup
            return fs::path();
        }
    }

#ifdef __APPLE__
    /*
    auto bundlePath = GetBundlePath();
    if (bundlePath) {
        return *bundlePath / "Contents/Resources/ProjectTemplates";
    }
    */
    // Fallback to engine path
    return fs::path(enginePathString) / "LarkEditor/ProjectTemplates";
#else
    return fs::path(enginePathString) / "LarkEditor/ProjectTemplates";
#endif
}

fs::path Utils::GetDefaultEnginePath() {
#ifdef _WIN32
    // Check common installation paths on Windows
    std::vector<fs::path> possiblePaths = {
        fs::current_path(),                    // Current directory
        fs::current_path().parent_path(),      // Parent directory
        fs::path("C:/Program Files/Lark"),
        fs::path("C:/Program Files (x86)/Lark"),
        fs::path("C:/Lark"),
        fs::path("D:/Lark"),
        fs::path("E:/Lark")
    };

    // Also check if we're running from a build directory
    fs::path exePath = fs::current_path();
    possiblePaths.push_back(exePath);
    possiblePaths.push_back(exePath.parent_path());
    possiblePaths.push_back(exePath.parent_path().parent_path());

    for (const auto& path : possiblePaths) {
        // Check if this looks like the engine directory
        if (fs::exists(path / "LarkEditor/ProjectTemplates")) {
            return path;
        }
    }

    // Return default even if it doesn't exist
    return fs::path("C:/Program Files/Lark");
#elif defined(__APPLE__)
    return fs::path("/Applications/Lark");
#else
    return fs::path("/opt/Lark");
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
        ImGui::Text("Lark Engine path is not set!");
        ImGui::Text("Please enter the path to the Lark Engine directory:");
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
                SetEnvironmentVariable("LARK_ENGINE", pathBuffer);
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