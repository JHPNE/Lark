#include "EditorApplication.h"

#include <fstream>

#include "../Utils/System/GlobalUndoRedo.h"
#include "../View/ComponentView.h"
#include "../View/FileDialog.h"
#include "../View/GeometryViewerView.h"
#include "../View/LoggerView.h"
#include "../View/ProjectBrowserView.h"
#include "../View/SceneView.h"
#include "../View/Style.h"
#include "Project/Project.h"
#include "View/PrimitiveMeshSelectionView.h"
#include "core/Loop.h"
#include "imgui_impl_opengl3.h"
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

    // Creating a window
    m_window = glfwCreateWindow(1280, 720, "Native Editor", nullptr, nullptr);
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

    // Initialize GeometryRenderer
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
    ApplyModernDarkStyle();

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

    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
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
    DrawMenuAndToolbar();

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

        PrimitiveMeshSelectionView::Get().SetActiveProject(loadedProject);
        PrimitiveMeshSelectionView::Get().Draw();
    }
}

void EditorApplication::DrawMenuAndToolbar()
{
    // Main menu bar
    if (ImGui::BeginMainMenuBar())
    {
        // File Menu
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Project", "Ctrl+N") ||
                ImGui::MenuItem("Open Project", "Ctrl+O"))
            {
                ProjectBrowserView::Get().GetShowState() = true;
            }
            if (ImGui::MenuItem("Save", "Ctrl+S"))
            {
                ProjectBrowserView::Get().GetLoadedProject()->Save();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4"))
            {
                m_Running = false;
            }
            // TODO REMOVE
            if (ImGui::MenuItem("Test Logger"))
            {
                Logger::Get().Log(MessageType::Info, "This is an info message");
                Logger::Get().Log(MessageType::Warning, "This is a warning message");
                Logger::Get().Log(MessageType::Error, "This is an error message", __FILE__,
                                  __FUNCTION__, __LINE__);
            }
            ImGui::EndMenu();
        }

        // Add separator line between menu and toolbar
        ImGui::SameLine(0, 20);

        // Undo/Redo Controls
        auto project = ProjectBrowserView::Get().GetLoadedProject();
        bool hasProject = project != nullptr;

        // Style the buttons to look like toolbar buttons
        ImGuiStyle &style = ImGui::GetStyle();
        float originalPadding = style.FramePadding.y;
        style.FramePadding.y = 2; // Reduce vertical padding

        // Undo Button
        if (ImGui::Button("Undo") && hasProject &&
            GlobalUndoRedo::Instance().GetUndoRedo().CanUndo())
        {
            GlobalUndoRedo::Instance().GetUndoRedo().Undo();
        }
        if (ImGui::IsItemHovered() && hasProject &&
            GlobalUndoRedo::Instance().GetUndoRedo().CanUndo())
        {
            const auto &undoList = GlobalUndoRedo::Instance().GetUndoRedo().GetUndoList();
            if (!undoList.empty())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Undo: %s", undoList.back()->GetName().c_str());
                ImGui::EndTooltip();
            }
        }

        ImGui::SameLine(0, 5);

        // Redo Button
        if (ImGui::Button("Redo") && hasProject &&
            GlobalUndoRedo::Instance().GetUndoRedo().CanRedo())
        {
            GlobalUndoRedo::Instance().GetUndoRedo().Redo();
        }
        if (ImGui::IsItemHovered() && hasProject &&
            GlobalUndoRedo::Instance().GetUndoRedo().CanRedo())
        {
            const auto &redoList = GlobalUndoRedo::Instance().GetUndoRedo().GetRedoList();
            if (!redoList.empty())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Redo: %s", redoList.front()->GetName().c_str());
                ImGui::EndTooltip();
            }
        }

        ImGui::SameLine(0, 15);

        if (hasProject)
        {
            // Script Creation Button
            if (ImGui::Button("Create Script"))
            {
                m_showScriptCreation = true;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Create new Python script");
                ImGui::EndTooltip();
            }

            ImGui::SameLine(0, 15);

            // Run Button
            if (ImGui::Button("Run"))
            {
                Loop::StartAsync();
            }

            // Run Button
            if (ImGui::Button("Stop"))
            {
                Loop::Stop();
            }
        }

        // No project tooltip
        if (!hasProject)
        {
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                ImGui::BeginTooltip();
                ImGui::Text("No project loaded");
                ImGui::EndTooltip();
            }
        }

        // Restore original style
        style.FramePadding.y = originalPadding;

        ImGui::EndMainMenuBar();

        // Script Creation Popup
        if (m_showScriptCreation)
        {
            ImGui::OpenPopup("Create Script");

            // Center the popup
            const ImGuiViewport *viewport = ImGui::GetMainViewport();
            ImVec2 center = viewport->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

            if (ImGui::BeginPopupModal("Create Script", &m_showScriptCreation,
                                       ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Enter script name:");
                ImGui::InputText("##ScriptName", m_scriptNameBuffer, sizeof(m_scriptNameBuffer));

                ImGui::Separator();

                if (ImGui::Button("Create", ImVec2(120, 0)))
                {
                    project->CreateNewScript(m_scriptNameBuffer);
                    m_showScriptCreation = false;
                    ImGui::CloseCurrentPopup();
                    // Reset buffer for next time
                    strcpy(m_scriptNameBuffer, "NewScript");
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0)))
                {
                    m_showScriptCreation = false;
                    ImGui::CloseCurrentPopup();
                    // Reset buffer for next time
                    strcpy(m_scriptNameBuffer, "NewScript");
                }

                ImGui::EndPopup();
            }
        }

        // Geometry Creation Popup
        static FileDialog file_dialog;
        if (m_showGeometryCreation)
        {
            if (file_dialog.Show(&m_showGeometryCreation))
            {
                const char *selectedPath = file_dialog.GetSelectedPathAsChar();
                if (selectedPath && strlen(selectedPath) > 0)
                {
                    GeometryInitializer geomInit;
                    geomInit.geometryName = "LoadedGeometry";
                    geomInit.geometryType = GeometryType::ObjImport;
                    geomInit.visible = true;
                    geomInit.geometrySource = selectedPath;
                    geomInit.meshType = content_tools::PrimitiveMeshType::cube; // Not used for OBJ

                    auto entity = project->GetActiveScene()->CreateEntityWithGeometry(
                        "LoadedGeometry", geomInit);
                    if (entity)
                    {
                        GeometryViewerView::Get().AddGeometry(entity->GetID());
                    }
                }
            }
        }
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
