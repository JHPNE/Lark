// LoggerView.cpp
#include "LoggerView.h"
#include "Style/CustomWidgets.h"
#include "Style/CustomWindow.h"
#include <imgui.h>

namespace LoggerView
{

using namespace LarkStyle;

void LoggerView::Draw()
{
    if (!m_show)
        return;

    CustomWindow::WindowConfig config;
    config.title = "Logger";
    config.icon = "📋";
    config.p_open = &m_show;
    config.allowDocking = true;
    config.defaultSize = ImVec2(600, 200);
    config.minSize = ImVec2(300, 150);

    if (CustomWindow::Begin("Logger", config)) {
        // Toolbar
        CustomWidgets::BeginToolbar("LoggerToolbar");
        {
            if (CustomWidgets::ToolbarButton("Clear", "Clear all messages")) {
                Logger::Get().Clear();
            }
            
            CustomWidgets::ToolbarSeparator();
            
            ImGui::Checkbox("Auto-scroll", &m_autoScroll);
            
            CustomWidgets::ToolbarSeparator();

            bool changed = false;
            changed |= ImGui::Checkbox("Info", &m_showInfo);
            ImGui::SameLine();
            changed |= ImGui::Checkbox("Warnings", &m_showWarnings);
            ImGui::SameLine();
            changed |= ImGui::Checkbox("Errors", &m_showErrors);

            if (changed) {
                int filter = 0;
                if (m_showInfo) filter |= static_cast<int>(MessageType::Info);
                if (m_showWarnings) filter |= static_cast<int>(MessageType::Warning);
                if (m_showErrors) filter |= static_cast<int>(MessageType::Error);
                Logger::Get().SetMessageFilter(filter);
            }
        }
        CustomWidgets::EndToolbar();

        CustomWidgets::Separator();

        // Messages panel
        CustomWidgets::BeginPanel("LogMessages", ImVec2(0, 0));
        {
            const auto &messages = Logger::Get().GetMessages();
            int messageFilter = Logger::Get().GetMessageFilter();

            for (const auto &msg : messages) {
                if (!(static_cast<int>(msg.type) & messageFilter))
                    continue;

                ImVec4 color;
                const char* prefix = "";
                
                switch (msg.type) {
                case MessageType::Info:
                    color = Colors::Text;
                    prefix = "[INFO]";
                    break;
                case MessageType::Warning:
                    color = Colors::AccentWarning;
                    prefix = "[WARN]";
                    break;
                case MessageType::Error:
                    color = Colors::AccentDanger;
                    prefix = "[ERROR]";
                    break;
                }

                ImGui::PushStyleColor(ImGuiCol_Text, color);

                // Time
                auto timeT = std::chrono::system_clock::to_time_t(msg.time);
                std::string timeStr = std::ctime(&timeT);
                timeStr = timeStr.substr(11, 8);

                ImGui::Text("[%s]", timeStr.c_str());
                ImGui::SameLine();
                ImGui::Text("%s", prefix);
                ImGui::SameLine();
                ImGui::TextWrapped("%s", msg.message.c_str());

                if (ImGui::IsItemHovered() && !msg.file.empty()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s(%d) : %s", msg.file.c_str(), msg.line, msg.caller.c_str());
                    ImGui::EndTooltip();
                }

                ImGui::PopStyleColor();
            }

            if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
        }
        CustomWidgets::EndPanel();
    }
    CustomWindow::End();
}

} // namespace LoggerView