#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "DeviceWatcher.h"
#include "Renderer.h"
#include "Scene.h"
#include "SelectableList.h"

class App
{
public:
    App();

    void Run();

private:
    struct SceneFactoryEntry
    {
        SceneFactoryEntry(const std::string &deviceName, const std::function<Scene *()> &factory)
            : DeviceName(deviceName), Factory(factory)
        {
        }

        std::string DeviceName;
        std::function<Scene *()> Factory;
    };

    Renderer m_Renderer;
    DeviceWatcher m_DeviceWatcher;
    SelectableList<SceneFactoryEntry> m_SceneFactories;
    std::unique_ptr<Scene> m_CurrentScene;
    SceneFactoryEntry *m_ActiveFactory;

    void Update();

    void Render();

    void SwitchScene();
};
