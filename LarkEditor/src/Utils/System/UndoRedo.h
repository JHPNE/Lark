#pragma once
#include <string>
#include <memory>
#include <vector>
#include <functional>

class IUndoRedo {
public:
	virtual ~IUndoRedo() = default;
	virtual std::string GetName() const = 0;
	virtual void Undo() = 0;
	virtual void Redo() = 0;
};

class UndoRedoAction : public IUndoRedo {
public:
	explicit UndoRedoAction(const std::string& name);
	UndoRedoAction(std::function<void()> undo, 
		std::function<void()> redo, 
		const std::string& name);

    template<typename T>
    UndoRedoAction(const std::string& property,
        T* instance,
        const typename T::value_type& undoValue,
        const typename T::value_type& redoValue,
        const std::string& name);

    // IUndoRedo interface
    std::string GetName() const override { return m_name; }
    void Undo() override;
    void Redo() override;

private:
    std::function<void()> m_undoAction;
    std::function<void()> m_redoAction;
    std::string m_name;
};

class UndoRedo {
public:
    UndoRedo() = default;
    ~UndoRedo() = default;

    // Prevent copying
    UndoRedo(const UndoRedo&) = delete;
    UndoRedo& operator=(const UndoRedo&) = delete;

    void Reset();
    void Undo();
    void Redo();
    void Add(std::shared_ptr<IUndoRedo> undoRedo);

    const std::vector<std::shared_ptr<IUndoRedo>>& GetUndoList() const { return m_undoList; }
    const std::vector<std::shared_ptr<IUndoRedo>>& GetRedoList() const { return m_redoList; }

    bool CanUndo() const { return !m_undoList.empty(); }
    bool CanRedo() const { return !m_redoList.empty(); }

private:
    std::vector<std::shared_ptr<IUndoRedo>> m_undoList;
    std::vector<std::shared_ptr<IUndoRedo>> m_redoList;
    bool m_enableAdd = true;
};