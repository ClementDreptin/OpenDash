#include <imgui.h>

#include <XexUtils.h>

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

    // Render a simple button.
    ImGui::Begin("Hello, world!");
    if (ImGui::Button("Button"))
        XexUtils::Log::Print("Button clicked");
    ImGui::End();

    // End the frame.
    m_Renderer.EndFrame();
}
