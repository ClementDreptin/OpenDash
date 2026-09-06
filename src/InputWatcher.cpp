#include <XexUtils.h>

#include "Event.h"
#include "InputWatcher.h"

void InputWatcher::Update()
{
    XexUtils::Input::Gamepad *pGamepad = XexUtils::Input::GetInput();
    if (pGamepad->PressedButtons)
        Emit(ButtonPressedEvent(*pGamepad));
}
