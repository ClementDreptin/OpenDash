#pragma once

#include <XexUtils.h>

#include "Event.h"

class InputWatcher : public EventEmitter
{
public:
    void Update();
};

class ButtonPressedEvent : public Event
{
public:
    ButtonPressedEvent(const XexUtils::Input::Gamepad &gamepad)
        : m_Gamepad(gamepad) {}

    const XexUtils::Input::Gamepad &GetGamepad() const { return m_Gamepad; }

    EVENT_CLASS_TYPE(EventType_ButtonPressed)

private:
    XexUtils::Input::Gamepad m_Gamepad;
};
