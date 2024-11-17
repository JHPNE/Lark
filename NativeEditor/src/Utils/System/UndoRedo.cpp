// UndoRedo.cpp
#include "UndoRedo.h"
#include <cassert>

UndoRedoAction::UndoRedoAction(const std::string& name)
    : m_name(name)
{
}

UndoRedoAction::UndoRedoAction(std::function<void()> undoAction,
    std::function<void()> redoAction,
    const std::string& name)
    : m_undoAction(std::move(undoAction))
    , m_redoAction(std::move(redoAction))
    , m_name(name)
{
    assert(m_undoAction && m_redoAction && "Actions cannot be null");
}

template<typename T>
UndoRedoAction::UndoRedoAction(const std::string& property,
    T* instance,
    const typename T::value_type& undoValue,
    const typename T::value_type& redoValue,
    const std::string& name)
    : m_name(name)
{
    assert(instance && "Instance cannot be null");

    m_undoAction = [instance, property, undoValue]() {
        instance->SetProperty(property, undoValue);
        };

    m_redoAction = [instance, property, redoValue]() {
        instance->SetProperty(property, redoValue);
        };
}

void UndoRedoAction::Undo()
{
    if (m_undoAction) {
        m_undoAction();
    }
}

void UndoRedoAction::Redo()
{
    if (m_redoAction) {
        m_redoAction();
    }
}

void UndoRedo::Reset()
{
    m_undoList.clear();
    m_redoList.clear();
}

void UndoRedo::Undo()
{
    if (m_undoList.empty()) {
        return;
    }

    auto action = m_undoList.back();
    m_undoList.pop_back();

    m_enableAdd = false;
    action->Undo();
    m_enableAdd = true;

    m_redoList.insert(m_redoList.begin(), action);
}

void UndoRedo::Redo()
{
    if (m_redoList.empty()) {
        return;
    }

    auto action = m_redoList.front();
    m_redoList.erase(m_redoList.begin());

    m_enableAdd = false;
    action->Redo();
    m_enableAdd = true;

    m_undoList.push_back(action);
}

void UndoRedo::Add(std::shared_ptr<IUndoRedo> undoRedo)
{
    if (!m_enableAdd) {
        return;
    }

    m_undoList.push_back(undoRedo);
    m_redoList.clear();
}