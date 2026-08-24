#include <imgui.h>

#include "App.h"

void App::Run()
{
    for (;;)
        Render();
}

void App::Render()
{
    // Start the frame.
    m_Renderer.StartFrame();

    // Render the games.
    m_GamesExplorer.Render();

    // End the frame.
    m_Renderer.EndFrame();
}
