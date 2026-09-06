#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "../Core/Scene.h"
#include "../Devices/DeviceWatcher.h"
#include "../Input/InputWatcher.h"
#include "../Renderer/Renderer.h"

class App
{
public:
    App();

    void Run();

private:
    struct SceneFactoryEntry
    {
        SceneFactoryEntry(const std::string &deviceName, const std::function<Scene *()> &factory)
            : SceneName(deviceName), Factory(factory) {}

        std::string SceneName;
        std::function<Scene *()> Factory;
    };

    Renderer m_Renderer;
    DeviceWatcher m_DeviceWatcher;
    InputWatcher m_InputWatcher;
    std::vector<SceneFactoryEntry> m_SceneFactories;
    std::unique_ptr<Scene> m_CurrentScene;
    std::string m_ActiveSceneName;

    void Update();

    void Render();

    void PropagateEvent(Event &event);

    void OnEvent(Event &event);

    bool OnDeviceChanged(DeviceChangedEvent &event);

    void SwitchScene(const SceneFactoryEntry &entry);
};
