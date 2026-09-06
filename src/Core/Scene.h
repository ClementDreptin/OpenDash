#pragma once

#include <XexUtils.h>

#include "Event.h"

class Scene
{
public:
    virtual ~Scene() {}

    virtual void Render() = 0;

    virtual void OnEvent(Event &event) {}
};
