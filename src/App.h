#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "Renderer.h"
#include "Scene.h"

class App
{
public:
    App();

    void Run();

private:
    Renderer m_Renderer;
    std::vector<std::function<Scene *()>> m_SceneFactories;
    std::unique_ptr<Scene> m_CurrentScene;
    size_t m_CurrentSceneIndex;

    void Update();

    void Render();

    void SwitchScene(size_t index);
};
