#include <XexUtils.h>
#include <algorithm>
#include <functional>
#include <imgui.h>
#include <memory>
#include <vector>

#include "../Core/Scene.h"
#include "../Devices/DeviceExplorer.h"
#include "../Devices/DeviceWatcher.h"
#include "../Devices/GamesExplorer.h"
#include "../Input/InputWatcher.h"
#include "../System/SystemInfo.h"
#include "../UI/Renderer.h"
#include "App.h"
#include "Event.h"
#include "Exceptions.h"

App::App()
{
    // Allow the event emitters to propagate their events to the rest of the app.
    auto propagateEvent = [this](Event &event) { PropagateEvent(event); };
    m_DeviceWatcher.SetEventCallback(propagateEvent);
    m_InputWatcher.SetEventCallback(propagateEvent);

    // Add the GamesExplorer if hdd:\Games directory is present.
    bool hasHdd = (XboxHardwareInfo->Flags & XBOX_HARDWARE_FLAG_HDD) != 0;
    if (hasHdd)
    {
        uint32_t gamesDirAttributes = GetFileAttributes("hdd:\\Games");
        bool hasGamesDir = gamesDirAttributes != 0xFFFFFFFF && (gamesDirAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        if (hasGamesDir)
            m_SceneFactories.emplace_back(SceneFactoryEntry("Games", []() -> Scene * { return new GamesExplorer(); }));
    }

    // Go through all the available devices and add a DeviceExplorer for each.
    const auto &devices = m_DeviceWatcher.GetDevices();
    for (size_t i = 0; i < devices.size(); i++)
    {
        const auto &device = devices[i];
        if (device.Available)
        {
            std::string deviceName = device.Name;
            bool readOnly = device.ReadOnly;
            m_SceneFactories.emplace_back(SceneFactoryEntry(deviceName, [deviceName, readOnly]() -> Scene * {
                return new DeviceExplorer(deviceName + "\\", readOnly);
            }));
        }
    }

    // Add the SystemInfo scene.
    m_SceneFactories.emplace_back(SceneFactoryEntry("System Info", []() -> Scene * { return new SystemInfo(); }));
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
    // Update the event emitters.
    m_DeviceWatcher.Update();
    m_InputWatcher.Update();
}

void App::Render()
{
    // Start the frame.
    m_Renderer.StartFrame();

    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove;

    // Create a window that takes the full safe area with no decoration except for a border.
    Renderer::Area safeArea = Renderer::GetSafeArea();
    ImGui::SetNextWindowPos(ImVec2(safeArea.Origin.x, safeArea.Origin.y));
    ImGui::SetNextWindowSize(ImVec2(safeArea.Width, safeArea.Height));
    ImGui::Begin("Main", nullptr, windowFlags);

    // Render a tab for each scene.
    if (ImGui::BeginTabBar("SceneTabs"))
    {
        for (size_t i = 0; i < m_SceneFactories.size(); i++)
        {
            const SceneFactoryEntry &entry = m_SceneFactories[i];

            if (ImGui::BeginTabItem(entry.SceneName.c_str()))
            {
                // Switch to the corresponding scene when entering a new tab.
                bool isCurrentlyActive = entry.SceneName == m_ActiveSceneName;
                if (!isCurrentlyActive)
                    SwitchScene(entry);

                ImGui::EndTabItem();
            }
        }

        ImGui::EndTabBar();
    }

    // Render the current scene.
    if (m_CurrentScene)
        m_CurrentScene->Render();

    ImGui::End();

    // End the frame.
    m_Renderer.EndFrame();
}

void App::PropagateEvent(Event &event)
{
    // Handle the event locally in this class first.
    OnEvent(event);
    if (event.Handled)
        return;

    // If the event wasn't handled by the App class, propagate it to the current scene.
    if (m_CurrentScene)
        m_CurrentScene->OnEvent(event);
}

void App::OnEvent(Event &event)
{
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<DeviceChangedEvent>([this](DeviceChangedEvent &e) { return OnDeviceChanged(e); });
}

bool App::OnDeviceChanged(DeviceChangedEvent &event)
{
    const DeviceInfo &deviceInfo = event.GetDeviceInfo();

    // If a device was inserted, append a DeviceExplorer for it to the list of scenes.
    if (deviceInfo.Available)
    {
        std::string deviceName = deviceInfo.Name;
        m_SceneFactories.emplace_back(SceneFactoryEntry(deviceName, [deviceName]() -> Scene * { return new DeviceExplorer(deviceName + "\\"); }));
    }
    // If a device was removed, remove its corresponding DeviceExplorer.
    else
    {
        auto toRemove = std::find_if(m_SceneFactories.begin(), m_SceneFactories.end(), [&](const SceneFactoryEntry &entry) {
            return entry.SceneName == deviceInfo.Name;
        });
        XASSERT(toRemove != m_SceneFactories.end());
        m_SceneFactories.erase(toRemove);

        // If the removed SceneFactoryEntry was for the active scene, destroy the scene.
        // A new scene will be recreated on the next frame based on which tab ImGui fell
        // back to after the removal.
        bool wasActive = deviceInfo.Name == m_ActiveSceneName;
        if (wasActive)
        {
            m_ActiveSceneName.clear();
            m_CurrentScene.reset();
        }
    }

    return false;
}

void App::SwitchScene(const SceneFactoryEntry &entry)
{
    // Destroy the previous scene and create the new one.
    m_ActiveSceneName = entry.SceneName;
    m_CurrentScene.reset(entry.Factory());
}
