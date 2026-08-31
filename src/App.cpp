#include <functional>
#include <memory>
#include <vector>
#include <imgui.h>
#include <XexUtils.h>

#include "App.h"
#include "Exceptions.h"
#include "DeviceExplorer.h"
#include "GamesExplorer.h"
#include "Renderer.h"
#include "Scene.h"

App::App()
    : m_CurrentSceneIndex(0)
{
    bool hasHdd = (XboxHardwareInfo->Flags & XBOX_HARDWARE_FLAG_HDD) != 0;
    if (hasHdd)
    {
        // Mount the HDD.
        // Collisions are expected when loading relaunching the app so it's fine.
        HRESULT hr = XexUtils::Fs::MountHdd();
        if (FAILED(hr) && hr != STATUS_OBJECT_NAME_COLLISION)
            throw Exception("[App]: Couldn't mount the HDD (%X).", hr);

        // Add the games explorer if hdd:\Games directory is present.
        uint32_t gamesDirAttributes = GetFileAttributes("hdd:\\Games");
        bool hasGamesDir = gamesDirAttributes != 0xFFFFFFFF && (gamesDirAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        if (hasGamesDir)
            m_SceneFactories.emplace_back([]() -> Scene * { return new GamesExplorer(); });

        // Add a device explorer for the hard drive.
        m_SceneFactories.emplace_back([]() -> Scene * { return new DeviceExplorer("hdd:\\"); });
    }
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

    // Update the current scene.
    m_CurrentScene->Update(pGamepad);
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
