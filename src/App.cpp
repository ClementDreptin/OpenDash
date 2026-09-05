#include <functional>
#include <memory>
#include <vector>
#include <XexUtils.h>

#include "App.h"
#include "Exceptions.h"
#include "DeviceExplorer.h"
#include "DeviceWatcher.h"
#include "GamesExplorer.h"
#include "Renderer.h"
#include "Scene.h"
#include "SelectableList.h"

App::App()
    : m_ActiveFactory(nullptr)
{
    bool hasHdd = (XboxHardwareInfo->Flags & XBOX_HARDWARE_FLAG_HDD) != 0;
    if (hasHdd)
    {
        // Add the games explorer if hdd:\Games directory is present.
        uint32_t gamesDirAttributes = GetFileAttributes("hdd:\\Games");
        bool hasGamesDir = gamesDirAttributes != 0xFFFFFFFF && (gamesDirAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        if (hasGamesDir)
            m_SceneFactories.PushBack(SceneFactoryEntry("", []() -> Scene * { return new GamesExplorer(); }));
    }

    // Go through all the available devices and add a DeviceExplorer for each.
    const auto &devices = m_DeviceWatcher.GetDevices();
    for (size_t i = 0; i < devices.size(); i++)
    {
        const auto &device = devices[i];
        if (device.Available)
        {
            std::string deviceName = device.Name;
            m_SceneFactories.PushBack(SceneFactoryEntry(deviceName, [deviceName]() -> Scene * { return new DeviceExplorer(deviceName + "\\"); }));
        }
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
    // Check for updates from the device watcher.
    auto deviceInfo = m_DeviceWatcher.Update();
    if (deviceInfo)
    {
        // If a device was inserted, append a DeviceExplorer for it to the list of scenes.
        if (deviceInfo->Available)
        {
            std::string deviceName = deviceInfo->Name;
            m_SceneFactories.PushBack(SceneFactoryEntry(deviceName, [deviceName]() -> Scene * { return new DeviceExplorer(deviceName + "\\"); }));
        }
        // If a device removed, remove its corresponding DeviceExplorer.
        else
        {
            m_SceneFactories.RemoveIf([&](const SceneFactoryEntry &entry) {
                return entry.DeviceName == deviceInfo->Name;
            });
        }
    }

    XexUtils::Input::Gamepad *pGamepad = XexUtils::Input::GetInput();

    // Switch scene with LB/RB.
    if (pGamepad->PressedButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)
        m_SceneFactories.SelectPrevious();
    else if (pGamepad->PressedButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
        m_SceneFactories.SelectNext();

    // Switch scene if requested or if we don't have an active scene, which is the case
    // on the first run.
    if (m_SceneFactories.GetSelected() != m_ActiveFactory || !m_CurrentScene)
        SwitchScene();

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

void App::SwitchScene()
{
    XASSERT(m_SceneFactories.GetSelected() != nullptr);

    // Destroy the previous scene and create the new one.
    m_ActiveFactory = m_SceneFactories.GetSelected();
    m_CurrentScene.reset(m_ActiveFactory->Factory());
}
