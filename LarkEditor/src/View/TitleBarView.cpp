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

void TitleBarView::Draw() {
    m_viewModel->Update();
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar |
                                   ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_MenuBar |
                                   ImGuiWindowFlags_NoNavFocus |
                                   ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoDocking;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, m_height));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, Colors::BackgroundDark);
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, Colors::BackgroundDark);

    if (ImGui::Begin("##TitleBar", nullptr, window_flags)) {
        if (ImGui::BeginMenuBar()) {
            DrawMenuBar();
            DrawWindowControls();
            ImGui::EndMenuBar();
        }
        
        HandleDragging();
    }
    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

void TitleBarView::DrawMenuBar() {
    float iconSize = 16.0f;

    // Adjust style so menus expand vertically
    float verticalPadding = (m_height - ImGui::GetTextLineHeight()) * 0.5f - 2.0f;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, verticalPadding));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Colors::AccentHover);

    // App icon
    ImGui::SetCursorPosY((m_height - iconSize) * 0.5f);
    ImGui::Image(nullptr, ImVec2(iconSize, iconSize));

    // Window title
    ImGui::SameLine();
    ImGui::SetCursorPosY((m_height - ImGui::GetTextLineHeight()) * 0.5f);
    ImGui::TextColored(Colors::TextBright, "%s", m_viewModel->WindowTitle.Get().c_str());

    ImGui::SameLine(0, 30);

    // Menus
    for (const auto& menu : m_viewModel->GetMenus()) {
        if (menu.isCompact) {
            bool enabled = !menu.items.empty() && menu.items[0].isEnabled();
            ImGui::BeginDisabled(!enabled);
            if (ImGui::BeginMenu(menu.label.c_str())) {
                if (ImGui::MenuItem(menu.items[0].label.c_str(),
                                   menu.items[0].shortcut.c_str())) {
                    if (menu.items[0].action) {
                        menu.items[0].action();
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndDisabled();
        } else {
            if (ImGui::BeginMenu(menu.label.c_str())) {
                for (const auto& item : menu.items) {
                    if (item.isSeparator) {
                        ImGui::Separator();
                    } else {
                        bool enabled = item.isEnabled ? item.isEnabled() : true;
                        if (ImGui::MenuItem(item.label.c_str(),
                                          item.shortcut.c_str(),
                                          false,
                                          enabled)) {
                            if (item.action) {
                                item.action();
                            }
                        }
                    }
                }
                ImGui::EndMenu();
            }
        }
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(); // remove custom padding
}

void TitleBarView::DrawWindowControls() {
    float buttonWidth = 46.0f;
    float buttonHeight = m_height;
    float totalWidth = buttonWidth * 3;
    
    ImGui::SameLine(ImGui::GetWindowWidth() - totalWidth - 8);
    ImGui::SetCursorPosY((m_height - buttonHeight) * 0.5f);
    
    // Custom style for window control buttons
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.2f));
    
    // Minimize
    if (ImGui::Button("─", ImVec2(buttonWidth, buttonHeight))) {
        m_viewModel->MinimizeCommand->Execute();
    }
    
    ImGui::SameLine();
    
    // Maximize/Restore
    const char* maxIcon = m_viewModel->IsMaximized.Get() ? "❐" : "□";
    if (ImGui::Button(maxIcon, ImVec2(buttonWidth, buttonHeight))) {
        m_viewModel->MaximizeCommand->Execute();
    }
    
    ImGui::SameLine();
    
    // Close - special styling
    ImGui::PopStyleColor(2); // Remove normal hover colors
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Colors::AccentDanger);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
    
    if (ImGui::Button("×", ImVec2(buttonWidth, buttonHeight))) {
        m_viewModel->CloseCommand->Execute();
    }
    
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
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