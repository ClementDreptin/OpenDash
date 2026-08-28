#include <imgui.h>
#include <XexUtils.h>

#include "App.h"
#include "DummyScene.h"
#include "GamesExplorer.h"

App::App()
    : m_CurrentSceneIndex(0)
{
    // Create the factories.
    m_SceneFactories.emplace_back([]() -> Scene * { return new DummyScene(); });
    m_SceneFactories.emplace_back([]() -> Scene * { return new GamesExplorer(); });
}

void App::Run()
{
    for (;;)
    {
        Update();

        Render();
    }
}

void App::Update()
{
    XexUtils::Input::Gamepad *pGamepad = XexUtils::Input::GetInput();

    // Switch scene with LB/RB.
    size_t newIndex = m_CurrentSceneIndex;
    if (pGamepad->PressedButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)
    {
        if (m_CurrentSceneIndex > 0)
            newIndex--;
    }
    else if (pGamepad->PressedButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
    {
        if (m_CurrentSceneIndex < m_SceneFactories.size() - 1)
            newIndex++;
    }

    // Switch scene if requested or if we don't have an active scene, which is the case
    // on the first run.
    if (newIndex != m_CurrentSceneIndex || !m_CurrentScene)
        SwitchScene(newIndex);
}

void App::Render()
{
    // Start the frame.
    m_Renderer.StartFrame();

    // Render the current scene.
    XASSERT(m_CurrentScene);
    m_CurrentScene->Render();

    // End the frame.
    m_Renderer.EndFrame();
}

void App::SwitchScene(size_t index)
{
    XASSERT(index < m_SceneFactories.size());

    // Destroy the previous scene and create the new one.
    m_CurrentSceneIndex = index;
    m_CurrentScene.reset(m_SceneFactories[index]());
}
