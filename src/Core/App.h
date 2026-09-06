#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "../Core/Scene.h"
#include "../Devices/DeviceWatcher.h"
#include "../Input/InputWatcher.h"
#include "../Renderer/Renderer.h"
#include "../Utils/SelectableList.h"

class App
{
public:
    App();

    void Run();

private:
    struct SceneFactoryEntry
    {
        SceneFactoryEntry(const std::string &deviceName, const std::function<Scene *()> &factory)
            : DeviceName(deviceName), Factory(factory) {}

        std::string DeviceName;
        std::function<Scene *()> Factory;
    };

    Renderer m_Renderer;
    DeviceWatcher m_DeviceWatcher;
    InputWatcher m_InputWatcher;
    SelectableList<SceneFactoryEntry> m_SceneFactories;
    std::unique_ptr<Scene> m_CurrentScene;
    SceneFactoryEntry *m_ActiveFactory;

    void Update();

    void Render();

    void PropagateEvent(Event &event);

    void OnEvent(Event &event);

    bool OnButtonPressed(ButtonPressedEvent &event);

    bool OnDeviceChanged(DeviceChangedEvent &event);

    void SwitchScene();
};
