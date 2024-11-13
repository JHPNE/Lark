#pragma once
#include <memory>

class Project; // Forward declaration instead of #include "Project.h"

class ComponentView {
public:
	static ComponentView& Get() {
		static ComponentView instance;
		return instance;
	}

	void Draw();
	bool& GetShowState() { return m_show; }
	void SetActiveProject(std::shared_ptr<Project> activeProject) { project = activeProject; }

private:
	ComponentView() = default;
	~ComponentView() = default;

	bool m_show = true;
	std::shared_ptr<Project> project;

};