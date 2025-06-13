// SelectionManager.h
#pragma once
#include <memory>
#include <unordered_set>
#include "GameEntity.h"
#include "../../Project/Scene.h"

class SelectionManager {
public:
    static SelectionManager& Get() {
        static SelectionManager instance;
        return instance;
    }

    void SelectEntity(std::shared_ptr<GameEntity> entity, bool isMultiSelect = false) {
        if (!entity) return;

        if (!isMultiSelect) {
            ClearSelection();
        }

        // Clear scene selection when selecting entities
        m_selectedScenes.clear();

        m_selectedEntities.insert(entity);
        entity->SetSelected(true);
    }

    void DeselectEntity(std::shared_ptr<GameEntity> entity) {
        if (!entity) return;

        if (m_selectedEntities.erase(entity) > 0) {
            entity->SetSelected(false);
        }
    }

    void SelectScene(std::shared_ptr<Scene> scene, bool isMultiSelect = false) {
        if (!scene) return;

        if (!isMultiSelect) {
            ClearSelection();
        }

        // Clear entity selection when selecting scenes
        for (auto& entity : m_selectedEntities) {
            entity->SetSelected(false);
        }
        m_selectedEntities.clear();

        m_selectedScenes.insert(scene);
    }

    void DeselectScene(std::shared_ptr<Scene> scene) {
        if (!scene) return;
        m_selectedScenes.erase(scene);
    }

    void ClearSelection() {
        // Clear entity selection and update their states
        for (auto& entity : m_selectedEntities) {
            entity->SetSelected(false);
        }
        m_selectedEntities.clear();

        // Clear scene selection
        m_selectedScenes.clear();
    }

    bool IsEntitySelected(std::shared_ptr<GameEntity> entity) const {
        if (!entity) return false;
        return m_selectedEntities.find(entity) != m_selectedEntities.end();
    }

    bool IsSceneSelected(std::shared_ptr<Scene> scene) const {
        if (!scene) return false;
        return m_selectedScenes.find(scene) != m_selectedScenes.end();
    }

    size_t GetSelectionCount() const {
        return m_selectedEntities.size() + m_selectedScenes.size();
    }

    const std::unordered_set<std::shared_ptr<GameEntity>>& GetSelectedEntities() const {
        return m_selectedEntities;
    }

    const std::unordered_set<std::shared_ptr<Scene>>& GetSelectedScenes() const {
        return m_selectedScenes;
    }

    bool HasMultipleSelections() const {
        return GetSelectionCount() > 1;
    }

private:
    SelectionManager() = default;
    ~SelectionManager() = default;

    SelectionManager(const SelectionManager&) = delete;
    SelectionManager& operator=(const SelectionManager&) = delete;

    std::unordered_set<std::shared_ptr<Scene>> m_selectedScenes;
    std::unordered_set<std::shared_ptr<GameEntity>> m_selectedEntities;
};