#pragma once
#include <memory>

class Project;
class SceneViewModel;

class SceneView
{
  public:
    static SceneView &Get()
    {
        static SceneView instance;
        return instance;
    }

    void Draw();
    bool &GetShowState() { return m_show; }
    void SetActiveProject(std::shared_ptr<Project> activeProject);

  private:
    SceneView();
    ~SceneView();

    void DrawSceneNode(const struct SceneNodeData& node);
    void DrawEntityContextMenu(uint32_t entityId);
    void DrawSceneContextMenu(uint32_t sceneId);

    bool m_show = true;
    std::unique_ptr<SceneViewModel> m_viewModel;
};
