#pragma once
#include "../ViewModels/ObservableProperty.h"
#include "../Commands/ICommand.h"
#include "../View/ProjectBrowserView.h"
#include "../View/ProjectSettingsView.h"
#include "../Project/Project.h"
#include "../Utils/System/GlobalUndoRedo.h"
#include "../Utils/Etc/Logger.h"
#include "../core/Loop.h"
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <memory>
#include <string>
#include <vector>

struct TitleBarMenuItem {
    std::string label;
    std::string shortcut;
    std::function<void()> action;
    std::function<bool()> isEnabled;
    bool isSeparator = false;
};

struct TitleBarMenu {
    std::string label;
    std::vector<TitleBarMenuItem> items;
    bool isCompact = false; // For single-item menus like Undo/Redo
};

class TitleBarViewModel {
public:
    // Window state
    ObservableProperty<bool> IsMaximized{false};
    ObservableProperty<bool> IsMinimized{false};
    ObservableProperty<std::string> WindowTitle{"Native Editor"};
    ObservableProperty<bool> HasProject{false};
    ObservableProperty<bool> IsModified{false};
    ObservableProperty<bool> ShowScriptCreation{false};

    // Menu state
    ObservableProperty<bool> CanUndo{false};
    ObservableProperty<bool> CanRedo{false};
    ObservableProperty<std::string> UndoText{"Undo"};
    ObservableProperty<std::string> RedoText{"Redo"};
    ObservableProperty<bool> IsRunning{false};

    // Commands
    std::unique_ptr<RelayCommand<>> MinimizeCommand;
    std::unique_ptr<RelayCommand<>> MaximizeCommand;
    std::unique_ptr<RelayCommand<>> CloseCommand;
    std::unique_ptr<RelayCommand<>> NewProjectCommand;
    std::unique_ptr<RelayCommand<>> OpenProjectCommand;
    std::unique_ptr<RelayCommand<>> SaveCommand;
    std::unique_ptr<RelayCommand<>> UndoCommand;
    std::unique_ptr<RelayCommand<>> RedoCommand;
    std::unique_ptr<RelayCommand<>> RunCommand;
    std::unique_ptr<RelayCommand<>> StopCommand;
    std::unique_ptr<RelayCommand<>> CreateScriptCommand;
    std::unique_ptr<RelayCommand<>> ExitCommand;
    std::unique_ptr<RelayCommand<>> ShowProjectSettingsCommand;

    TitleBarViewModel(GLFWwindow* window) : m_window(window) {
        InitializeCommands();
        SubscribeToPropertyChanges();
    }

    void Update() {
        UpdateProjectState();
        UpdateUndoRedoState();
        UpdateWindowTitle();
        UpdateWindowState();
    }

    std::vector<TitleBarMenu> GetMenus() const {
        return {
            TitleBarMenu{
                "File",
                {
                    {"New Project", "Ctrl+N", [this]() { NewProjectCommand->Execute(); }, []() { return true; }},
                    {"Open Project", "Ctrl+O", [this]() { OpenProjectCommand->Execute(); }, []() { return true; }},
                    {"Save", "Ctrl+S", [this]() { SaveCommand->Execute(); }, [this]() { return SaveCommand->CanExecute(); }},
                    {"", "", nullptr, nullptr, true}, // Separator
                    {"Exit", "Alt+F4", [this]() { ExitCommand->Execute(); }, []() { return true; }}
                }
            },
            TitleBarMenu{
                "Undo",
                {
                    {UndoText.Get(), "Ctrl+Z", [this]() { UndoCommand->Execute(); }, [this]() { return UndoCommand->CanExecute(); }}
                },
                true // Compact menu
            },
            TitleBarMenu{
                "Redo",
                {
                    {RedoText.Get(), "Ctrl+Y", [this]() { RedoCommand->Execute(); }, [this]() { return RedoCommand->CanExecute(); }}
                },
                true // Compact menu
            },
            TitleBarMenu{
                "Create Script",
                {
                    {"New Python Script", "", [this]() { CreateScriptCommand->Execute(); }, [this]() { return CreateScriptCommand->CanExecute(); }}
                },
                true
            },
            TitleBarMenu{
                "Run",
                {
                    {"Start", "F5", [this]() { RunCommand->Execute(); }, [this]() { return RunCommand->CanExecute(); }}
                },
                true
            },
            TitleBarMenu{
                "Stop",
                {
                    {"Stop", "Shift+F5", [this]() { StopCommand->Execute(); }, [this]() { return StopCommand->CanExecute(); }}
                },
                true
            }
        };
    }

    bool IsWindowBeingDragged() const { return m_isDragging; }

    void StartDragging(double mouseX, double mouseY) {
        if (m_isDragging) return;

        m_isDragging = true;
        m_dragStartX = mouseX;
        m_dragStartY = mouseY;
        glfwGetWindowPos(m_window, &m_windowStartX, &m_windowStartY);
    }

    void UpdateDragging(double mouseX, double mouseY) {
        if (!m_isDragging) return;

        int newX = m_windowStartX + static_cast<int>(mouseX - m_dragStartX);
        int newY = m_windowStartY + static_cast<int>(mouseY - m_dragStartY);
        glfwSetWindowPos(m_window, newX, newY);
    }

    void StopDragging() {
        m_isDragging = false;
    }

private:
    GLFWwindow* m_window;
    std::weak_ptr<Project> m_currentProject;

    // Dragging state
    bool m_isDragging = false;
    double m_dragStartX = 0;
    double m_dragStartY = 0;
    int m_windowStartX = 0;
    int m_windowStartY = 0;

    void InitializeCommands() {
        MinimizeCommand = std::make_unique<RelayCommand<>>(
            [this]() { glfwIconifyWindow(m_window); }
        );

        MaximizeCommand = std::make_unique<RelayCommand<>>(
            [this]() {
                if (IsMaximized.Get()) {
                    glfwRestoreWindow(m_window);
                } else {
                    glfwMaximizeWindow(m_window);
                }
                IsMaximized = !IsMaximized.Get();
            }
        );

        CloseCommand = std::make_unique<RelayCommand<>>(
            [this]() { glfwSetWindowShouldClose(m_window, GLFW_TRUE); }
        );

        ExitCommand = std::make_unique<RelayCommand<>>(
            [this]() { glfwSetWindowShouldClose(m_window, GLFW_TRUE); }
        );

        NewProjectCommand = std::make_unique<RelayCommand<>>(
            []() {
                // close current project etc
                    /*
                auto& browser = ProjectBrowserView::Get();
                browser.GetShowState() = true;
                browser.SetNewProjectMode(true);
                */
            }
        );

        OpenProjectCommand = std::make_unique<RelayCommand<>>(
            []() {
                /*
                auto& browser = ProjectBrowserView::Get();
                browser.GetShowState() = true;
                browser.SetNewProjectMode(false);
                */
            }
        );

        SaveCommand = std::make_unique<RelayCommand<>>(
            [this]() {
                if (auto project = m_currentProject.lock()) {
                    if (project->Save()) {
                        Logger::Get().Log(MessageType::Info, "Project saved: " + project->GetName());
                        UpdateStatus("Project saved");
                    } else {
                        Logger::Get().Log(MessageType::Error, "Failed to save project");
                        UpdateStatus("Save failed");
                    }
                }
            },
            [this]() { return !m_currentProject.expired(); }
        );

        UndoCommand = std::make_unique<RelayCommand<>>(
            []() {
                GlobalUndoRedo::Instance().GetUndoRedo().Undo();
            },
            []() {
                return GlobalUndoRedo::Instance().GetUndoRedo().CanUndo();
            }
        );

        RedoCommand = std::make_unique<RelayCommand<>>(
            []() {
                GlobalUndoRedo::Instance().GetUndoRedo().Redo();
            },
            []() {
                return GlobalUndoRedo::Instance().GetUndoRedo().CanRedo();
            }
        );

        RunCommand = std::make_unique<RelayCommand<>>(
            [this]() {
                Loop::StartAsync();
                IsRunning = true;
                Logger::Get().Log(MessageType::Info, "Simulation started");
            },
            [this]() {
                return !IsRunning.Get() && !m_currentProject.expired();
            }
        );

        StopCommand = std::make_unique<RelayCommand<>>(
            [this]() {
                Loop::Stop();
                IsRunning = false;
                Logger::Get().Log(MessageType::Info, "Simulation stopped");
            },
            [this]() {
                return IsRunning.Get();
            }
        );

        CreateScriptCommand = std::make_unique<RelayCommand<>>(
            [this]() {
                ShowScriptCreation = true;
            },
            [this]() {
                return !m_currentProject.expired();
            }
        );

        ShowProjectSettingsCommand = std::make_unique<RelayCommand<>>(
            []() {
                ProjectSettingsView::Get().GetShowState() = true;
            },
            [this]() { return !m_currentProject.expired(); }
        );
    }

    void SubscribeToPropertyChanges() {
        // Subscribe to project changes if needed
        HasProject.Subscribe([this](const bool& oldVal, const bool& newVal) {
            if (newVal) {
                UpdateWindowTitle();
            }
        });

        IsModified.Subscribe([this](const bool& oldVal, const bool& newVal) {
            UpdateWindowTitle();
        });
    }

    void UpdateProjectState() {
        auto project = ProjectBrowserView::Get().GetLoadedProject();
        if (project) {
            m_currentProject = project;
            HasProject = true;
            IsModified = project->IsModified();
        } else {
            m_currentProject.reset();
            HasProject = false;
            IsModified = false;
        }
    }

    void UpdateUndoRedoState() {
        auto& undoRedo = GlobalUndoRedo::Instance().GetUndoRedo();

        CanUndo = undoRedo.CanUndo();
        CanRedo = undoRedo.CanRedo();

        if (CanUndo.Get() && !undoRedo.GetUndoList().empty()) {
            UndoText = "Undo: " + undoRedo.GetUndoList().back()->GetName();
        } else {
            UndoText = std::string("Undo");
        }

        if (CanRedo.Get() && !undoRedo.GetRedoList().empty()) {
            RedoText = "Redo: " + undoRedo.GetRedoList().front()->GetName();
        } else {
            RedoText = std::string("Redo");
        }
    }

    void UpdateWindowTitle() {
        std::string title = "Native Editor";
        if (auto project = m_currentProject.lock()) {
            title += " - " + project->GetName();
            if (IsModified.Get()) {
                title += " *";
            }
        }
        WindowTitle = title;

        // Also update the actual window title
        glfwSetWindowTitle(m_window, title.c_str());
    }

    void UpdateWindowState() {
        IsMaximized = glfwGetWindowAttrib(m_window, GLFW_MAXIMIZED);
        IsMinimized = glfwGetWindowAttrib(m_window, GLFW_ICONIFIED);
    }

    void UpdateStatus(const std::string& message) {
        // Could emit an event or update a status property
        Logger::Get().Log(MessageType::Info, message);
    }
};