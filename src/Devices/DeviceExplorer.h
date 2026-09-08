#pragma once

#include <XexUtils.h>
#include <vector>
#include <xtl.h>

#include "../Core/Scene.h"
#include "../Input/InputWatcher.h"
#include "../Renderer/Renderer.h"

class DeviceExplorer : public Scene
{
public:
    DeviceExplorer(const XexUtils::Fs::Path &baseDir);

    void Render() override;

    void OnEvent(Event &event) override;

private:
    XexUtils::Fs::Path m_CurrentDir;
    std::vector<XexUtils::Fs::File> m_Files;
    size_t m_SelectedFileIndex;
    Texture m_DirectoryTexture;
    Texture m_FileTexture;
    Texture m_XexTexture;
    std::string m_ErrorMessage;

    void RenderFileList();

    void RenderActionBar();

    bool OnButtonPressed(ButtonPressedEvent &event);

    void ChangeDirectory(const XexUtils::Fs::Path &newDir);
};
