#include <imgui.h>

#include "App.h"

void App::Run()
{
    for (;;)
        Render();
}

#define CHAR_A "\xEF\x81\x81"

void App::Render()
{
    // Start the frame.
    m_Renderer.StartFrame();

    // Render a simple text with an icon.
    ImGui::Begin("Hello, world!");
    ImGui::Text("Some text with the " CHAR_A " button");
    ImGui::End();

    // End the frame.
    m_Renderer.EndFrame();
}
