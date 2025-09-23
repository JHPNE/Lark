#include "EditorApplication.h"

#include <fstream>

#include "../Utils/System/GlobalUndoRedo.h"
#include "../View/ComponentView.h"
#include "../View/FileDialog.h"
#include "../View/GeometryViewerView.h"
#include "../View/LoggerView.h"
#include "../View/ProjectBrowserView.h"
#include "../View/SceneView.h"
#include "Project/Project.h"
#include "core/Loop.h"
#include "imgui_impl_opengl3.h"
#include "Rendering/GeometryRenderer.h"
#include "Style/CustomWidgets.h"
#include "View/ProjectSettingsView.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <iostream>

namespace editor
{

bool EditorApplication::Initialize()
{
    // Initializing GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

// Change GLSL version based on platform
#ifdef __APPLE__
    const char *glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GL_TRUE);
    glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_TRUE);
#else
    const char *glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    // Creating a window
    m_window = glfwCreateWindow(1280, 720, "Lark Editor", nullptr, nullptr);
    if (m_window == nullptr)
    {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    if (!GeometryRenderer::Initialize())
    {
        std::cerr << "Failed to initialize geometry renderer" << std::endl;
        return false;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows

    // Setup Dear ImGui style
    LarkStyle::CustomWidgets::Initialize();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look
    // identical to regular ones.
    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Initialize title bar (but don't show it yet)
    m_titleBar = std::make_unique<TitleBarView>(m_window);

    // Start with project browser
    m_state = AppState::ProjectBrowser;
    m_Running = true;
    return true;
}

void EditorApplication::Run()
{
    while (m_Running && !glfwWindowShouldClose(m_window))
    {
        // Poll events
        glfwPollEvents();

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Draw based on state
        switch (m_state)
        {
        case AppState::ProjectBrowser:
            DrawProjectBrowser();
            break;

        case AppState::Editor:
            DrawEditor();
            break;
        }

        // End frame
        EndFrame();
    }
}

void EditorApplication::DrawProjectBrowser()
{
    // Just draw the project browser - no title bar, no dockspace
    ProjectBrowserView::Get().Draw();

    // Check for transition
    if (ProjectBrowserView::Get().ShouldTransition())
    {
        m_state = AppState::Editor;
        auto project = ProjectBrowserView::Get().GetLoadedProject();
        InitializeEditorViews(project);
    }
}

void EditorApplication::DrawEditor()
{
    // Create docking environment with title bar
    CreateDockingEnvironment();

    // Draw all editor views
    Update();

    // End the dockspace
    ImGui::End();
}

void EditorApplication::CreateDockingEnvironment()
{
    // Draw title bar
    m_titleBar->Draw();

    // Setup dockspace
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking |
                                    ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus |
                                    ImGuiWindowFlags_NoNavFocus |
                                    ImGuiWindowFlags_NoBackground;

    ImGuiViewport *viewport = ImGui::GetMainViewport();
    float titleBarHeight = m_titleBar->GetHeight();

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + titleBarHeight));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - titleBarHeight));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
}


void EditorApplication::InitializeEditorViews(std::shared_ptr<Project> project)
{
    if (!project)
    {
        std::cerr << "Error: Trying to initialize editor views with null project" << std::endl;
        return;
    }

    SceneView::Get().SetActiveProject(project);
    ComponentView::Get().SetActiveProject(project);
    GeometryViewerView::Get().SetActiveProject(project);
    ProjectSettingsView::Get().SetActiveProject(project);

    // Also set for project browser view so it knows the project is loaded
    //ProjectBrowserView::Get().SetLoadedProject(project);
}


void EditorApplication::Update()
{
    // Logger window
    LoggerView::LoggerView::Get().Draw();

    // Scene Window
    SceneView::Get().Draw();

    // Component Window
    ComponentView::Get().Draw();

    // Render Window
    GeometryViewerView::Get().Draw();

    // Project Settings
    ProjectSettingsView::Get().Draw();
}

void EditorApplication::EndFrame()
{
    if (m_state == AppState::Editor)
    {
        //ImGui::End(); // only end the dockspace if we are in Editor mode
    }

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(m_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(m_clearColor.x, m_clearColor.y, m_clearColor.z, m_clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow *backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    glfwSwapBuffers(m_window);
}

void EditorApplication::Shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

} // namespace editor
