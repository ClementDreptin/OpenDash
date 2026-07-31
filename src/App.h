#pragma once

#include "Renderer.h"

class App
{
public:
    void Run();

private:
    Renderer m_Renderer;

private:
    void Render();
};
