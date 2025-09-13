#pragma once
#include <vector>
#include <functional>

template<typename T>
class ObservableProperty
{
public:
    using ChangeHandler = std::function<void(const T&, const T&)>;

    ObservableProperty() = default;
    ObservableProperty(const T& initial) : m_value(initial) {}

    template<typename... Args>
    ObservableProperty(Args... args) : m_value(args...) {}

    const T& Get() const { return m_value; }

    void Set(const T& value)
    {
        if (m_value != value) {
            T oldValue = m_value;
            m_value = value;
            NotifyChanged(oldValue, m_value);
        }
    }

    void Subscribe(ChangeHandler handler)
    {
        m_handlers.push_back(handler);
    }

    // Operator overloads for convenience
    operator const T&() const { return m_value; }
    ObservableProperty& operator=(const T& value)
    {
        Set(value);
        return *this;
    }

private:
    T m_value{};
    std::vector<ChangeHandler> m_handlers;

    void NotifyChanged(const T& oldValue, const T& newValue)
    {
        for (auto& handler : m_handlers) {
            if (handler) {
                handler(oldValue, newValue);
            }
        }
    }
};
