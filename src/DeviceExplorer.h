#pragma once

#include <xtl.h>
#include <vector>
#include <XexUtils.h>

#include "InputWatcher.h"
#include "Renderer.h"
#include "Scene.h"

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

    bool OnButtonPressed(ButtonPressedEvent &event);

    void ChangeDirectory(const XexUtils::Fs::Path &newDir);
};
