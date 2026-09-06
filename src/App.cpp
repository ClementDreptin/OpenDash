#include <functional>
#include <memory>
#include <vector>
#include <XexUtils.h>

#include "App.h"
#include "DeviceExplorer.h"
#include "DeviceWatcher.h"
#include "Event.h"
#include "Exceptions.h"
#include "GamesExplorer.h"
#include "InputWatcher.h"
#include "Renderer.h"
#include "Scene.h"
#include "SelectableList.h"

App::App()
    : m_ActiveFactory(nullptr)
{
    // Allow the event emitters to propagate their events to the rest of the app.
    auto propagateEvent = [this](Event &event) { PropagateEvent(event); };
    m_DeviceWatcher.SetEventCallback(propagateEvent);
    m_InputWatcher.SetEventCallback(propagateEvent);

    bool hasHdd = (XboxHardwareInfo->Flags & XBOX_HARDWARE_FLAG_HDD) != 0;
    if (hasHdd)
    {
        // Add the GamesExplorer if hdd:\Games directory is present.
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
    // We don't have an active scene on the first run so we immediately switch to the
    // default one.
    if (!m_CurrentScene)
        SwitchScene();

    // Update the event emitters.
    m_DeviceWatcher.Update();
    m_InputWatcher.Update();
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

void App::PropagateEvent(Event &event)
{
    XASSERT(m_CurrentScene);

    // Handle the event locally in this class first.
    OnEvent(event);
    if (event.Handled)
        return;

    // If the event wasn't handled by the App class, propagate it to the current scene.
    m_CurrentScene->OnEvent(event);
}

void App::OnEvent(Event &event)
{
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<ButtonPressedEvent>([this](ButtonPressedEvent &e) { return OnButtonPressed(e); });
    dispatcher.Dispatch<DeviceChangedEvent>([this](DeviceChangedEvent &e) { return OnDeviceChanged(e); });
}

bool App::OnButtonPressed(ButtonPressedEvent &event)
{
    const XexUtils::Input::Gamepad &gamepad = event.GetGamepad();

    // Switch scene with LB/RB.
    if (gamepad.PressedButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)
        m_SceneFactories.SelectPrevious();
    else if (gamepad.PressedButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
        m_SceneFactories.SelectNext();

    // Switch scene if requested.
    if (m_SceneFactories.GetSelected() != m_ActiveFactory)
    {
        SwitchScene();
        return true;
    }

    return false;
}

bool App::OnDeviceChanged(DeviceChangedEvent &event)
{
    const DeviceInfo &deviceInfo = event.GetDeviceInfo();

    // If a device was inserted, append a DeviceExplorer for it to the list of scenes.
    if (deviceInfo.Available)
    {
        std::string deviceName = deviceInfo.Name;
        m_SceneFactories.PushBack(SceneFactoryEntry(deviceName, [deviceName]() -> Scene * { return new DeviceExplorer(deviceName + "\\"); }));
    }
    // If a device was removed, remove its corresponding DeviceExplorer.
    else
    {
        m_SceneFactories.RemoveIf([&](const SceneFactoryEntry &entry) {
            return entry.DeviceName == deviceInfo.Name;
        });

        // If the removed DeviceExplorer was the current scene, switch to the closest
        // available scene.
        if (m_SceneFactories.GetSelected() != m_ActiveFactory)
            SwitchScene();
    }

    return false;
}

void App::SwitchScene()
{
    XASSERT(m_SceneFactories.GetSelected() != nullptr);

    // Destroy the previous scene and create the new one.
    m_ActiveFactory = m_SceneFactories.GetSelected();
    m_CurrentScene.reset(m_ActiveFactory->Factory());
}
