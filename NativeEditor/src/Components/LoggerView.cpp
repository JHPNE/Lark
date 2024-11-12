#pragma once
#include "LoggerView.h"
#include <imgui.h>


namespace LoggerView {
	void LoggerView::Draw()
	{
		if (!m_show)
			return;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
		window_flags |= ImGuiWindowFlags_NoCollapse;

		if (ImGui::Begin("Logger", &m_show, window_flags))
		{
			if (ImGui::Button("Clear")) Logger::Get().Clear();
			ImGui::SameLine();
			ImGui::Checkbox("Auto-scroll", &m_autoScroll);
			ImGui::SameLine();

			bool changed = false;
			changed |= ImGui::Checkbox("Info", &m_showInfo);
			ImGui::SameLine();
			changed |= ImGui::Checkbox("Warnings", &m_showWarnings);
			ImGui::SameLine();		
			changed |= ImGui::Checkbox("Errors", &m_showErrors);

			if (changed)
			{
				int filter = 0;
				if (m_showInfo) filter |= static_cast<int>(MessageType::Info);
				if (m_showWarnings) filter |= static_cast<int>(MessageType::Warning);
				if (m_showErrors) filter |= static_cast<int>(MessageType::Error);
				Logger::Get().SetMessageFilter(filter);
			}

			ImGui::Separator();

			ImGui::BeginChild("ScrollingRegion",
				ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

			const auto& messages = Logger::Get().GetMessages();
			int messageFilter = Logger::Get().GetMessageFilter();

			for (const auto& msg : messages)
			{
				if (!(static_cast<int>(msg.type) & messageFilter))
					continue;

				ImVec4 color;
				//TODO: Get color from a theme
				switch (msg.type)
				{
					case MessageType::Info: color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break;
					case MessageType::Warning: color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); break;
					case MessageType::Error: color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); break;
				}

				ImGui::PushStyleColor(ImGuiCol_Text, color);

				auto timeT = std::chrono::system_clock::to_time_t(msg.time);
				std::string timeStr = std::ctime(&timeT);
				timeStr = timeStr.substr(11, 8);

				ImGui::TextUnformatted(timeStr.c_str());
				ImGui::SameLine();
				ImGui::TextUnformatted(msg.message.c_str());

				if (ImGui::IsItemHovered() && !msg.file.empty())
				{
					ImGui::BeginTooltip();
					ImGui::Text("%s(%d) : %s", msg.file.c_str(), msg.line, msg.caller.c_str());
					ImGui::EndTooltip();
				}

				ImGui::PopStyleColor();

			}

			if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
				ImGui::SetScrollHereY(1.0f);

			ImGui::EndChild();

		}
		ImGui::End();
	}


}
