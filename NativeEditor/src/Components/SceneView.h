#pragma once
#include <memory>

class Project; // Forward declaration instead of #include "Project.h"

class SceneView {
public:
	static SceneView& Get() {
		static SceneView instance;
		return instance;
	}

	void Draw();
	bool& GetShowState() { return m_show; }
	void SetActiveProject(std::shared_ptr<Project> activeProject) { project = activeProject; }

private:
	SceneView() = default;
	~SceneView() = default;

	bool m_show = true;
	std::shared_ptr<Project> project;

};
