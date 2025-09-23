#pragma once
#include "Geometry/Geometry.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <memory>

#include "View/TitleBarView.h"

namespace editor
{
class EditorApplication
{

  public:
    enum class AppState
    {
        ProjectBrowser,
        Editor
    };

    static EditorApplication &Get()
    {
        static EditorApplication instance;
        return instance;
    }

    bool Initialize();
    void Run();
    void Shutdown();

    GLFWwindow *GetWindow() const { return m_window; }
    ImVec4 GetClearColor() const { return m_clearColor; }

  private:
    EditorApplication() = default;
    ~EditorApplication() = default;

    void DrawEditor();
    void CreateDockingEnvironment();
    void DrawProjectBrowser();
    auto InitializeEditorViews(std::shared_ptr<Project> project)-> void;
    void EndFrame();
    void Update();

    GLFWwindow *m_window = nullptr;
    ImVec4 m_clearColor = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    bool m_Running = false;
    AppState m_state = AppState::ProjectBrowser;

    std::unique_ptr<TitleBarView> m_titleBar;
};
} // namespace editor
