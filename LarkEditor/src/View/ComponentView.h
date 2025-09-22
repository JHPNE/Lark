// LarkEditor/src/View/ComponentView.h
#pragma once
#include <memory>

class Project;
class ComponentViewModel;

class ComponentView {
public:
    static ComponentView& Get() {
        static ComponentView instance;
        return instance;
    }

    void Draw();
    bool& GetShowState() { return m_show; }
    void SetActiveProject(std::shared_ptr<Project> activeProject);

private:
    ComponentView();
    ~ComponentView();

    void DrawSingleSelection();
    void DrawMultiSelection();
    void DrawTransformComponent();
    void DrawScriptComponent();
    void DrawGeometryComponent();
    void DrawPhysicsComponent();
    void DrawDroneComponent();
    void DrawAddComponentButton();

    bool m_show = true;
    std::unique_ptr<ComponentViewModel> m_viewModel;
};