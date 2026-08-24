#pragma once

#include "GamesExplorer.h"
#include "Renderer.h"

class App
{
public:
    void Run();

private:
    GamesExplorer m_GamesExplorer;
    Renderer m_Renderer;

private:
    void Render();
};
