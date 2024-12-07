#pragma once
#include <memory>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include "Geometry/Geometry.h"

namespace editor {
	class EditorApplication {
	public:
		static EditorApplication& Get() {
			static EditorApplication instance;
			return instance;
		}

		bool Initialize();
		void Run();
		void Shutdown();

		GLFWwindow* GetWindow() const { return m_window; }
		ImVec4 GetClearColor() const { return m_clearColor; }

	private:
		EditorApplication() = default;
		~EditorApplication() = default;


		void BeginFrame();
		void EndFrame();
		void Update();

		void DrawMenuAndToolbar();

		GLFWwindow* m_window = nullptr;
		ImVec4 m_clearColor = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		bool m_Running = false;

		void CreateNewScript(const char* scriptName);

		// Add member for script creation popup state
		bool m_showScriptCreation = false;
		char m_scriptNameBuffer[256] = "NewScript";

		bool m_showGeometryCreation = false;
		char m_geometryNameBuffer[256] = "C:/Users/yeeezy/Documents/monke.obj";

		std::unique_ptr<drosim::editor::Geometry> m_geometry;
	};
}
