#pragma once
#include <functional>

class ICommand
{
public:
    virtual ~ICommand() = default;
    virtual void Execute() = 0;
    virtual bool CanExecute() const { return true; }
};

template<typename T = void>
class RelayCommand : public ICommand
{
public:
    using ExecuteFunc = std::function<void(T)>;
    using CanExecuteFunc = std::function<bool(T)>;

    RelayCommand(ExecuteFunc execute, CanExecuteFunc canExecute = nullptr)
        : m_execute(execute), m_canExecute(canExecute) {}

    void Execute() override { Execute(T{}); }
    void Execute(T parameter) { if (CanExecute(parameter)) m_execute(parameter); }

    bool CanExecute() const override { return CanExecute(T{}); }
    bool CanExecute(T parameter) const
    {
        return m_canExecute ? m_canExecute(parameter) : true;
    }

private:
    ExecuteFunc m_execute;
    CanExecuteFunc m_canExecute;
};

// Specialization for parameterless commands
template<>
class RelayCommand<void> : public ICommand
{
public:
    using ExecuteFunc = std::function<void()>;
    using CanExecuteFunc = std::function<bool()>;

    RelayCommand(ExecuteFunc execute, CanExecuteFunc canExecute = nullptr)
        : m_execute(execute), m_canExecute(canExecute) {}

    void Execute() override { if (CanExecute()) m_execute(); }
    bool CanExecute() const override { return m_canExecute ? m_canExecute() : true; }

private:
    ExecuteFunc m_execute;
    CanExecuteFunc m_canExecute;
};