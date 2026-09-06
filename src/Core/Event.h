#pragma once

#include <functional>

// This event system is heavily inspired by TheCherno's application architecture example:
// https://github.com/TheCherno/Architecture/blob/main/Core/Source/Core/Event.h

typedef enum _EventType
{
    EventType_None,
    EventType_ButtonPressed,
    EventType_DeviceChanged,
} EventType;

#define EVENT_CLASS_TYPE(type) \
    static EventType GetStaticType() \
    { \
        return type; \
    } \
    virtual EventType GetEventType() const override \
    { \
        return GetStaticType(); \
    }

class Event
{
public:
    Event()
        : Handled(false) {}

    virtual ~Event() {}

    virtual EventType GetEventType() const = 0;

    bool Handled;
};

class EventEmitter
{
public:
    typedef std::function<void(Event &)> Callback;

    void SetEventCallback(const Callback &callback) { m_Callback = callback; }

protected:
    void Emit(Event &event)
    {
        if (m_Callback)
            m_Callback(event);
    }

private:
    Callback m_Callback;
};

class EventDispatcher
{
public:
    EventDispatcher(Event &event)
        : m_Event(event) {}

    template<typename T>
    bool Dispatch(const std::function<bool(T &)> &handler)
    {
        if (m_Event.GetEventType() == T::GetStaticType() && !m_Event.Handled)
        {
            m_Event.Handled = handler(*dynamic_cast<T *>(&m_Event));
            return true;
        }
        return false;
    }

private:
    Event &m_Event;

    EventDispatcher(const EventDispatcher &other);
    EventDispatcher operator=(const EventDispatcher &other);
};
