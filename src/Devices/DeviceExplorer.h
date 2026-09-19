#pragma once

#include <XexUtils.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <xtl.h>

#include "../Core/Event.h"
#include "../Core/Scene.h"
#include "../Input/InputWatcher.h"
#include "../UI/NativeKeyboard.h"
#include "../UI/Renderer.h"
#include "../Utils/AsyncFileOperation.h"
#include "DeviceWatcher.h"

class DeviceExplorer : public Scene
{
public:
    DeviceExplorer(const DeviceInfo &deviceInfo);

    ~DeviceExplorer();

    void Render() override;

    void OnEvent(Event &event) override;

private:
    DeviceInfo m_DeviceInfo;
    XexUtils::Fs::Path m_CurrentDir;
    std::vector<XexUtils::Fs::File> m_Files;
    size_t m_SelectedFileIndex;
    Texture m_DirectoryTexture;
    Texture m_FileTexture;
    Texture m_XexTexture;
    std::string m_ErrorMessage;
    bool m_ShouldFocusFirstItem;
    bool m_ShouldOpenMenu;
    bool m_ShouldOpenOptions;
    bool m_ShouldOpenDeviceInfo;
    NativeKeyboard m_RenameKeyboard;
    NativeKeyboard m_CreateDirKeyboard;
    std::unique_ptr<AsyncFileOperation> m_ActiveOperation;
    bool m_IsDvdAvailable;
    uint64_t m_FreeBytes;
    uint64_t m_TotalBytes;

    void RenderCurrentDir();

    void RenderFileList();

    void RenderOptions();

    void RenderMenu();

    void RenderActionBar();

    void RenderProgress();

    void RenderDeviceInfo();

    bool OnButtonPressed(ButtonPressedEvent &event);

    void ChangeDir(const XexUtils::Fs::Path &dirPath);

    void CreateDir(const XexUtils::Fs::Path &dirPath);

    void DeleteFile(const XexUtils::Fs::Path &filePath);

    void DeleteDir(const XexUtils::Fs::Path &dirPath);

    void RenameFile(const XexUtils::Fs::Path &oldPath, const XexUtils::Fs::Path &newPath);

    void Paste();

    void CopyDvd();

    void RefreshFileList();

private:
    typedef enum _ClipboardAction
    {
        ClipboardAction_None,
        ClipboardAction_Copy,
        ClipboardAction_Cut,
    } ClipboardAction;

    struct Clipboard
    {
        ClipboardAction Action;
        XexUtils::Fs::Path Path;

        void Cut(const XexUtils::Fs::Path &path);

        void Copy(const XexUtils::Fs::Path &path);

        void Clear();
    };

    static Clipboard s_Clipboard;

    static bool IsDvdAvailable();

    static std::string FormatBytesAsFileSize(uint64_t bytes, size_t decimalPlaces = 2);

    static std::string TimeToString(time_t time);
};
