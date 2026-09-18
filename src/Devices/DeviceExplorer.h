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

class DeviceExplorer : public Scene
{
public:
    DeviceExplorer(const XexUtils::Fs::Path &baseDir, bool readOnly = false);

    ~DeviceExplorer();

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
    bool m_ShouldFocusFirstItem;
    bool m_ShouldOpenMenu;
    bool m_ShouldOpenOptions;
    NativeKeyboard m_RenameKeyboard;
    NativeKeyboard m_CreateDirKeyboard;
    std::unique_ptr<AsyncFileOperation> m_ActiveOperation;
    bool m_ReadOnly;
    bool m_IsDvdAvailable;

    void RenderFileList();

    void RenderOptions();

    void RenderMenu();

    void RenderActionBar();

    void RenderProgress();

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

    static std::string FormatBytesAsMegabytes(uint64_t bytes, size_t decimalPlaces = 2);
};
