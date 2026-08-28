#pragma once

class Scene
{
public:
    virtual ~Scene() {}

    virtual void Render() = 0;
};
