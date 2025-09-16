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
#else
    const char *glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
#endif

    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GL_TRUE);
#endif

    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    // Creating a window
    m_window = glfwCreateWindow(1280, 720, "Native Editor", nullptr, nullptr);
    if (m_window == nullptr)
    {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // Enable vsync

    m_titleBar = std::make_unique<TitleBarView>(m_window);

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
    // ImGui::StyleColorsDark();
    //ApplyModernDarkStyle();
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

    m_Running = true;
    return true;
}

void EditorApplication::Run()
{
    while (m_Running && !glfwWindowShouldClose(m_window))
    {
        BeginFrame();
        Update();
        EndFrame();
    }
}

void EditorApplication::BeginFrame()
{
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Create the docking environment
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus |
                                    ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

    m_titleBar->Draw();

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

void EditorApplication::Update()
{
    // Logger window
    LoggerView::LoggerView::Get().Draw();

    // Project Browser Window
    ProjectBrowserView::Get().Draw();

    // Scene and Component Windows
    auto loadedProject = ProjectBrowserView::Get().GetLoadedProject();
    if (loadedProject)
    {
        // Scene Window
        SceneView::Get().SetActiveProject(loadedProject);
        SceneView::Get().Draw();

        // Component Window
        ComponentView::Get().SetActiveProject(loadedProject);
        ComponentView::Get().Draw();

        GeometryViewerView::Get().SetActiveProject(loadedProject);
        GeometryViewerView::Get().Draw();
    }
}

void EditorApplication::EndFrame()
{
    ImGui::End(); // End the dockspace

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
