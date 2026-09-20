#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "../Core/Event.h"
#include "../Core/Scene.h"
#include "../Devices/DeviceWatcher.h"
#include "../Input/InputWatcher.h"
#include "../UI/Renderer.h"

class App
{
public:
    App();

    void Run();

private:
    typedef enum _SceneGroup
    {
        SceneGroup_Games,
        SceneGroup_Devices,
        SceneGroup_System,
    } SceneGroup;

    struct SceneFactoryEntry
    {
        SceneFactoryEntry(const std::string &deviceName, SceneGroup group, const std::function<Scene *()> &factory)
            : SceneName(deviceName), Group(group), Factory(factory) {}

        std::string SceneName;
        SceneGroup Group;
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

    void AddScene(const SceneFactoryEntry &entry);

    void SwitchScene(const SceneFactoryEntry &entry);
};
