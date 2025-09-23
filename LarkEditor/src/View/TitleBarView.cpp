#include "TitleBarView.h"
#include "../Style/Theme.h"
#include "../Style/CustomWidgets.h"
#include <imgui.h>
#include <imgui_internal.h>

using namespace LarkStyle;

TitleBarView::TitleBarView(GLFWwindow* window) 
    : m_window(window), m_viewModel(std::make_unique<TitleBarViewModel>(window)) {
}

TitleBarView::~TitleBarView() = default;

void TitleBarView::Draw()
{
    m_viewModel->Update();
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar |
                                   ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoNavFocus |
                                   ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoDocking |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, m_height));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, Colors::BackgroundDark);

    if (ImGui::Begin("##TitleBar", nullptr, window_flags)) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 window_pos = ImGui::GetWindowPos();
        ImVec2 window_size = ImGui::GetWindowSize();

        // Draw background
        draw_list->AddRectFilled(
            window_pos,
            ImVec2(window_pos.x + window_size.x, window_pos.y + m_height),
            ImGui::GetColorU32(Colors::BackgroundDark)
        );

        // Draw content
        DrawContent();

        // Handle dragging
        HandleDragging();
    }
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

void TitleBarView::DrawContent()
{
    ImVec2 window_size = ImGui::GetWindowSize();

    // Set cursor position for menu items (vertically centered)
    float content_height = ImGui::GetTextLineHeightWithSpacing();
    float vertical_offset = (m_height - content_height) * 0.5f;

    ImGui::SetCursorPos(ImVec2(10, vertical_offset));

    // App icon (optional)
    float iconSize = 20.0f;
    ImGui::SetCursorPosY((m_height - iconSize) * 0.5f);
    ImGui::SetCursorPosX(10);

    // You can add an icon here if you have one
    // ImGui::Image(nullptr, ImVec2(iconSize, iconSize));
    // ImGui::SameLine();

    // Window title
    ImGui::SetCursorPosY((m_height - ImGui::GetTextLineHeight()) * 0.5f);
    ImGui::TextColored(Colors::TextBright, "%s", m_viewModel->WindowTitle.Get().c_str());

    // Menu items
    ImGui::SameLine(0, 30);
    ImGui::SetCursorPosY(vertical_offset);

    // Use child window for menu area to prevent overflow
    ImGui::BeginChild("MenuArea", ImVec2(window_size.x - 200, m_height), false,
                      ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);

    DrawMenuItems();

    ImGui::EndChild();

    // Draw window controls on the right
    DrawWindowControls();
}

void TitleBarView::DrawMenuItems() {
    float vertical_padding = (m_height - ImGui::GetFrameHeight()) * 0.5f;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, vertical_padding));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.2f));

    ImGui::SetCursorPosY((m_height - ImGui::GetFrameHeight()) * 0.5f);

    // File menu
    if (ImGui::Button("File")) {
        ImGui::OpenPopup("FileMenu");
    }

    ImGui::SameLine();
    if (ImGui::BeginPopup("FileMenu")) {
        if (ImGui::MenuItem("New Project", "Ctrl+N")) {
            m_viewModel->NewProjectCommand->Execute();
        }
        if (ImGui::MenuItem("Open Project", "Ctrl+O")) {
            m_viewModel->OpenProjectCommand->Execute();
        }
        if (ImGui::MenuItem("Save", "Ctrl+S", false, m_viewModel->HasProject.Get())) {
            m_viewModel->SaveCommand->Execute();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit", "Alt+F4")) {
            m_viewModel->ExitCommand->Execute();
        }
        ImGui::EndPopup();
    }

    // Undo button
    ImGui::SameLine();
    bool canUndo = m_viewModel->CanUndo.Get();
    if (!canUndo) {
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDisabled);
    }
    if (ImGui::Button("Undo") && canUndo) {
        m_viewModel->UndoCommand->Execute();
    }
    if (!canUndo) {
        ImGui::PopStyleColor();
    }

    // Redo button
    ImGui::SameLine();
    bool canRedo = m_viewModel->CanRedo.Get();
    if (!canRedo) {
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDisabled);
    }
    if (ImGui::Button("Redo") && canRedo) {
        m_viewModel->RedoCommand->Execute();
    }
    if (!canRedo) {
        ImGui::PopStyleColor();
    }

    // Other buttons
    if (m_viewModel->HasProject.Get()) {
        ImGui::SameLine();
        if (ImGui::Button("Create Script")) {
            m_viewModel->CreateScriptCommand->Execute();
        }

        ImGui::SameLine();
        if (!m_viewModel->IsRunning.Get()) {
            if (ImGui::Button("Run")) {
                m_viewModel->RunCommand->Execute();
            }
        } else {
            if (ImGui::Button("Stop")) {
                m_viewModel->StopCommand->Execute();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Settings")) {
            m_viewModel->ShowProjectSettingsCommand->Execute();
        }
    }

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}

void TitleBarView::DrawWindowControls() {
    ImVec2 window_size = ImGui::GetWindowSize();

    float buttonWidth = 46.0f;
    float buttonHeight = m_height;
    float totalWidth = buttonWidth * 3;

    // Position at the right edge
    ImGui::SetCursorPos(ImVec2(window_size.x - totalWidth, 0));

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));

    // Create a child window for the controls to ensure they're visible
    ImGui::BeginChild("WindowControls", ImVec2(totalWidth, buttonHeight), false,
                      ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);

    // Minimize button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.2f));
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::Text);

    ImGui::PushFont(ImGui::GetFont()); // Ensure font is set

    if (ImGui::Button("_", ImVec2(buttonWidth, buttonHeight))) {
        m_viewModel->MinimizeCommand->Execute();
    }

    // Maximize/Restore button
    ImGui::SameLine(0, 0);
    const char* maxIcon = m_viewModel->IsMaximized.Get() ? "◱" : "□";
    if (ImGui::Button(maxIcon, ImVec2(buttonWidth, buttonHeight))) {
        m_viewModel->MaximizeCommand->Execute();
    }

    // Close button with red hover
    ImGui::SameLine(0, 0);
    ImGui::PopStyleColor(2); // Remove hover colors
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));

    if (ImGui::Button("X", ImVec2(buttonWidth, buttonHeight))) {
        m_viewModel->CloseCommand->Execute();
    }

    ImGui::PopFont();
    ImGui::PopStyleColor(5);

    ImGui::EndChild();

    ImGui::PopStyleVar(3);
}

void TitleBarView::HandleDragging() {
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    
    // Check if mouse is in draggable area (title bar but not on menu items)
    bool inDragArea = mousePos.y >= windowPos.y && 
                     mousePos.y <= windowPos.y + m_height &&
                     !ImGui::IsAnyItemHovered();
    
    if (inDragArea && ImGui::IsMouseClicked(0))
    {
        m_isDragging = true;
        mousePos = ImGui::GetMousePos();
        m_dragStartX = mousePos.x;
        m_dragStartY = mousePos.y;
        glfwGetWindowPos(m_window, &m_windowStartX, &m_windowStartY);
    }
    
    if (m_isDragging) {
        if (ImGui::IsMouseReleased(0)) {
            m_isDragging = false;
        } else if (ImGui::IsMouseDragging(0)) {
            ImVec2 mousePos = ImGui::GetMousePos();
            int newX = m_windowStartX + (int)(mousePos.x - m_dragStartX);
            int newY = m_windowStartY + (int)(mousePos.y - m_dragStartY);
            static float smoothX = 0, smoothY = 0;
            smoothX = smoothX * 0.85f + newX * 0.15f;
            smoothY = smoothY * 0.85f + newY * 0.15f;
            glfwSetWindowPos(m_window, (int)smoothX, (int)smoothY);
        }
    }
    
    // Double-click to maximize
    if (inDragArea && ImGui::IsMouseDoubleClicked(0)) {
        m_viewModel->MaximizeCommand->Execute();
    }
}