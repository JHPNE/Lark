#pragma once
#include <functional>
#include <vector>
#include <unordered_set>
#include <memory>

class SelectionService
{
public:
    using SelectionChangedHandler = std::function<void(uint32_t oldId, uint32_t newId)>;
    using MultiSelectionChangedHandler = std::function<void(const std::unordered_set<uint32_t>&)>;

    static SelectionService& Get()
    {
        static SelectionService instance;
        return instance;
    };

    // Single Selection
    void SelectEntity(uint32_t entityId, bool addToSelection = false)
    {
        if (!addToSelection) {
            uint32_t oldId = m_selectedEntityId;
            m_selectedEntityIds.clear();
            m_selectedEntityId = entityId;
            m_selectedEntityIds.insert(entityId);

            NotifySelectionChanged(oldId, entityId);
            NotifyMultiSelectionChanged();
        }
        else
        {
            // Add to selection
            m_selectedEntityIds.insert(entityId);
            if (m_selectedEntityIds.size() == 1)
            {
                m_selectedEntityId = entityId;
            }
            NotifyMultiSelectionChanged();
        }
    }

    void DeselectEntity(uint32_t entityId)
    {
        m_selectedEntityIds.erase(entityId);
        if (m_selectedEntityId == entityId)
        {
            m_selectedEntityId = m_selectedEntityIds.empty() ?
                static_cast<uint32_t>(-1) : *m_selectedEntityIds.begin();
            NotifySelectionChanged(entityId, m_selectedEntityId);
        }
        NotifyMultiSelectionChanged();
    }

    void ClearSelection()
    {
        uint32_t oldId = m_selectedEntityId;
        m_selectedEntityIds.clear();
        m_selectedEntityId = static_cast<uint32_t>(-1);
        NotifySelectionChanged(oldId, m_selectedEntityId);
        NotifyMultiSelectionChanged();
    }

    uint32_t GetSelectedEntity() const { return m_selectedEntityId; }
    const std::unordered_set<uint32_t>& GetSelectedEntities() const { return m_selectedEntityIds; }
    bool IsSelected(uint32_t entityId) const
    {
        return m_selectedEntityIds.find(entityId) != m_selectedEntityIds.end();
    }
    bool HasSelection() const { return !m_selectedEntityIds.empty(); }
    bool HasMultipleSelection() const { return m_selectedEntityIds.size() > 1; }

    // Subscribe to changes
    void SubscribeToSelectionChange(SelectionChangedHandler handler)
    {
        m_selectionHandlers.push_back(handler);
    }

    void SubscribeToMultiSelectionChange(MultiSelectionChangedHandler handler)
    {
        m_multiSelectionHandlers.push_back(handler);
    }

private:
    SelectionService() : m_selectedEntityId(static_cast<uint32_t>(-1)) {}

    void NotifySelectionChanged(uint32_t oldId, uint32_t newId)
    {
        for (auto& handler : m_selectionHandlers)
        {
            if (handler) handler(oldId, newId);
        }
    }

    void NotifyMultiSelectionChanged()
    {
        for (auto& handler : m_multiSelectionHandlers)
        {
            if (handler) handler(m_selectedEntityIds);
        }
    }

    uint32_t m_selectedEntityId;
    std::unordered_set<uint32_t> m_selectedEntityIds;
    std::vector<SelectionChangedHandler> m_selectionHandlers;
    std::vector<MultiSelectionChangedHandler> m_multiSelectionHandlers;
};