#pragma once

#include <XexUtils.h>

class Scene
{
public:
    virtual ~Scene() {}

    virtual void Update(XexUtils::Input::Gamepad *pGamepad) {}

    virtual void Render() = 0;
};
