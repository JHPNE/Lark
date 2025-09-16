#pragma once
#include "../ViewModels/TitleBarViewModel.h"
#include <memory>
#include <GLFW/glfw3.h>

class TitleBarView {
public:
    TitleBarView(GLFWwindow* window);
    ~TitleBarView();
    
    void Draw();
    float GetHeight() const { return m_height; }
    
private:
    void DrawMenuBar();
    void DrawWindowControls();
    void HandleDragging();
    
    std::unique_ptr<TitleBarViewModel> m_viewModel;
    GLFWwindow* m_window;
    float m_height = 48.0f;
    
    // Dragging state
    bool m_isDragging = false;
    double m_dragStartX = 0;
    double m_dragStartY = 0;
    int m_windowStartX = 0;
    int m_windowStartY = 0;
};