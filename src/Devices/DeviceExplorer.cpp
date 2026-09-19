#include <XexUtils.h>
#include <cstdint>
#include <imgui.h>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <xtl.h>

#include "../Core/Exceptions.h"
#include "../Core/Scene.h"
#include "../Input/InputWatcher.h"
#include "../UI/Renderer.h"
#include "../UI/Theme.h"
#include "../Utils/AsyncFileOperation.h"
#include "../Utils/ScopeGuard.h"
#include "DeviceExplorer.h"
#include "DeviceWatcher.h"

DeviceExplorer::Clipboard DeviceExplorer::s_Clipboard;

DeviceExplorer::DeviceExplorer(const DeviceInfo &deviceInfo)
    : m_DeviceInfo(deviceInfo),
      m_SelectedFileIndex(0),
      m_DirectoryTexture("game:\\assets\\images\\directory.png"),
      m_FileTexture("game:\\assets\\images\\file.png"),
      m_XexTexture("game:\\assets\\images\\xex.png"),
      m_ShouldFocusFirstItem(false),
      m_ShouldOpenMenu(false),
      m_ShouldOpenOptions(false),
      m_ShouldOpenDeviceInfo(false),
      m_FreeBytes(0),
      m_TotalBytes(0)
{
    // The DVD is a special device because, unlike the USB, we can't detect when a DVD
    // is inserted or removed because opening the disc tray while the app is running shuts
    // down the app. If we want to use the DVD, it needs to be in the disc tray before the
    // app starts, so only checking for it's presence on init is fine.
    m_IsDvdAvailable = IsDvdAvailable();

    // Get the available space on the device.
    XexUtils::Fs::Path rootPath = m_DeviceInfo.Name + '\\';
    BOOL success = GetDiskFreeSpaceEx(
        rootPath.c_str(),
        nullptr,
        reinterpret_cast<ULARGE_INTEGER *>(&m_TotalBytes),
        reinterpret_cast<ULARGE_INTEGER *>(&m_FreeBytes)
    );
    if (!success)
        throw Exception("[DeviceExplorer]: Couldn't get the free space of %s (%i).", rootPath.c_str(), GetLastError());

    ChangeDir(rootPath);
}

DeviceExplorer::~DeviceExplorer()
{
    if (m_ActiveOperation)
        m_ActiveOperation->RequestCancel();
}

void DeviceExplorer::Render()
{
    RenderCurrentDir();

    RenderFileList();

    RenderOptions();

    RenderMenu();

    RenderActionBar();

    RenderProgress();

    RenderDeviceInfo();
}

void DeviceExplorer::OnEvent(Event &event)
{
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<ButtonPressedEvent>([this](ButtonPressedEvent &e) { return OnButtonPressed(e); });
}

void DeviceExplorer::RenderCurrentDir()
{
    ImGui::Text("%s", m_CurrentDir.c_str());
    ImGui::Separator();
}

void DeviceExplorer::RenderFileList()
{
    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NavFlattened |
        ImGuiWindowFlags_AlwaysUseWindowPadding;

    // Setup a child window with extra padding for the alignment.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
    float listHeight = ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y;
    ImGui::BeginChild("File list", ImVec2(0.0f, -listHeight), false, windowFlags);

    // Automatically end this window when this scope ends.
    auto endWindowGuard = MakeScopeGuard([]() {
        ImGui::EndChild();
        ImGui::PopStyleVar();
    });

    // State to keep across renders.
    static bool changeDirectoryRequested = false;

    // Render the error message if there is one.
    if (!m_ErrorMessage.empty())
    {
        ImGui::TextColored(Theme::Colors::Red, m_ErrorMessage.c_str());
        return;
    }

    // If the directory is empty, just render a placeholder text.
    if (m_Files.empty())
    {
        ImGui::Text("This directory is empty.");
        return;
    }

    // Render the list of files.
    for (size_t i = 0; i < m_Files.size(); i++)
    {
        // Create a selectable with an icon in it.

        const auto &file = m_Files[i];

        // Get the appropriate texture based on the file type.
        bool isDir = (file.Attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        bool isXex = file.FullPath.Extension() == ".xex";
        const Texture &texture = isDir ? m_DirectoryTexture : isXex ? m_XexTexture
                                                                    : m_FileTexture;

        // Save the cursor position before creating the selectable.
        ImVec2 cursorPos = ImGui::GetCursorPos();

        // Force nav focus onto the first selectable if it's the first frame after chaging
        // directory. Without this, the internal ImGui cursor still has the position from
        // the previous directory, which might not even exist if the new directory has
        // less files than the previous one.
        if (i == 0 && m_ShouldFocusFirstItem)
        {
            ImGui::SetKeyboardFocusHere();
            m_ShouldFocusFirstItem = false;
        }

        // Create a selectable with an invisible text. It's invisible because it
        // starts with "##".
        XexUtils::Fs::Path filename = file.FullPath.Filename();
        std::string label = "##" + filename.String();
        if (ImGui::Selectable(label.c_str(), m_SelectedFileIndex == i, 0, ImVec2(0.0f, texture.GetHeight())))
        {
            if (isDir)
                changeDirectoryRequested = true;
            else if (isXex)
                XLaunchNewImage(file.FullPath.c_str(), 0);
        }

        if (ImGui::IsItemFocused())
            m_SelectedFileIndex = i;

        // Move the cursor back to where it was before creating the selectable so that
        // the next thing we push is at the beginning of the selectable.
        ImGui::SetCursorPos(cursorPos);

        // Render the appropriate icon.
        ImGui::Image(texture.GetHandle(), ImVec2(texture.GetWidth(), texture.GetHeight()));

        // Vertically align the text with the middle of the icon.
        ImGui::SameLine();
        ImGui::SetCursorPosY(cursorPos.y + (texture.GetHeight() - ImGui::GetTextLineHeight()) * 0.5f);
        ImGui::Text(filename.c_str());

        // Render the file size.
        float availableWidth = ImGui::GetContentRegionAvail().x;
        if (!isDir)
        {
            ImGui::SameLine(availableWidth * 0.75f);
            ImGui::Text(FormatBytesAsFileSize(file.Size, 1).c_str());
        }

        // Render the last modification date.
        ImGui::SameLine(availableWidth * 0.85f);
        ImGui::Text(TimeToString(file.LastWriteTime).c_str());
    }

    // Change directory if requested.
    if (changeDirectoryRequested)
    {
        changeDirectoryRequested = false;
        ChangeDir(m_Files[m_SelectedFileIndex].FullPath);
    }
}

void DeviceExplorer::RenderOptions()
{
    if (m_Files.empty())
        return;

    // Open the options popup if requested.
    if (m_ShouldOpenOptions)
    {
        ImGui::OpenPopup("Options");
        m_ShouldOpenOptions = false;
    }

    const XexUtils::Fs::File &file = m_Files[m_SelectedFileIndex];
    XexUtils::Fs::Path filename = file.FullPath.Filename();
    bool shouldOpenConfirm = false;

    // Update the keyboard while it's open.
    if (m_RenameKeyboard.GetState() == NativeKeyboard::State_Pending)
        m_RenameKeyboard.Update();

    bool fileRenamed = false;
    if (m_RenameKeyboard.GetState() == NativeKeyboard::State_Success)
    {
        // Reset the state of the keyboard so that this if only runs once.
        m_RenameKeyboard.Reset();

        try
        {
            // Rename the file.
            RenameFile(file.FullPath, m_CurrentDir / m_RenameKeyboard.GetResult());
            RefreshFileList();
            fileRenamed = true;
        }
        catch (const Exception &exception)
        {
            XexUtils::Xam::XNotify(exception.what(), XexUtils::Xam::XNOTIFYUI_TYPE_AVOID_REVIEW);
        }
    }

    // Begin the popup.
    if (ImGui::BeginPopup("Options"))
    {
        ImVec2 buttonSize(ImGui::GetFontSize() * 4.0f, 0.0f);

        if (ImGui::Button("Copy", buttonSize))
        {
            s_Clipboard.Copy(file.FullPath);
            ImGui::CloseCurrentPopup();
        }

        // Actions that are not available for read only devices.
        if (!m_DeviceInfo.ReadOnly)
        {
            if (ImGui::Button("Cut", buttonSize))
            {
                s_Clipboard.Cut(file.FullPath);
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::Button("Rename", buttonSize))
                m_RenameKeyboard.Show(
                    "Rename",
                    XexUtils::Formatter::Format("Rename %s.", filename.c_str()),
                    filename.c_str()
                );

            if (ImGui::Button("Delete", buttonSize))
                shouldOpenConfirm = true;
        }

        // If the file rename was successful, close this popup.
        if (fileRenamed)
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    // Open the confirm modal if requested.
    if (shouldOpenConfirm)
    {
        ImGui::OpenPopup("Confirm");
        shouldOpenConfirm = false;
    }

    // Begin the confirm modal.
    if (ImGui::BeginPopupModal("Confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Are you sure you want to delete %s?", filename.c_str());
        ImGui::NewLine();

        // Align the buttons to the right of the modal.
        ImVec2 buttonSize(ImGui::GetFontSize() * 4.0f, 0.0f);
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float groupWidth = buttonSize.x * 2.0f + spacing;
        float availableWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availableWidth - groupWidth);

        if (ImGui::Button("Yes", buttonSize))
        {
            try
            {
                // Delete the file or directory.
                if (file.Attributes & FILE_ATTRIBUTE_DIRECTORY)
                    DeleteDir(file.FullPath);
                else
                    DeleteFile(file.FullPath);

                RefreshFileList();
                ImGui::CloseCurrentPopup();
            }
            catch (const Exception &exception)
            {
                XexUtils::Xam::XNotify(exception.what(), XexUtils::Xam::XNOTIFYUI_TYPE_AVOID_REVIEW);
            }
        }

        ImGui::SameLine();

        // Select the "No" button by default.
        if (ImGui::Button("No", buttonSize))
            ImGui::CloseCurrentPopup();
        ImGui::SetItemDefaultFocus();

        ImGui::EndPopup();
    }
}

void DeviceExplorer::RenderMenu()
{
    // Open the menu popup if requested.
    if (m_ShouldOpenMenu)
    {
        ImGui::OpenPopup("Menu");
        m_ShouldOpenMenu = false;
    }

    // Update the keyboard while it's open.
    if (m_CreateDirKeyboard.GetState() == NativeKeyboard::State_Pending)
        m_CreateDirKeyboard.Update();

    bool directoryCreated = false;
    if (m_CreateDirKeyboard.GetState() == NativeKeyboard::State_Success)
    {
        // Reset the state of the keyboard so that this if only runs once.
        m_CreateDirKeyboard.Reset();

        try
        {
            // Create the directory.
            XexUtils::Fs::Path newDir = m_CurrentDir / m_CreateDirKeyboard.GetResult();
            CreateDir(newDir);
            RefreshFileList();
            directoryCreated = true;
        }
        catch (const Exception &exception)
        {
            XexUtils::Xam::XNotify(exception.what(), XexUtils::Xam::XNOTIFYUI_TYPE_AVOID_REVIEW);
        }
    }

    // Stick the popup to the right side of the parent window and centered vertically.
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 windowPadding = ImGui::GetStyle().WindowPadding;
    ImVec2 anchorPos(windowPos.x + windowSize.x - windowPadding.x, windowPos.y + windowSize.y * 0.5f);
    ImGui::SetNextWindowPos(anchorPos, ImGuiCond_Always, ImVec2(1.0f, 0.5f));

    // Begin the popup.
    if (ImGui::BeginPopup("Menu"))
    {
        ImVec2 buttonSize(ImGui::GetFontSize() * 7.0f, 0.0f);

        if (ImGui::Button("Create directory", buttonSize))
            m_CreateDirKeyboard.Show(
                "Create directory",
                XexUtils::Formatter::Format("Create a directory in %s.", m_CurrentDir.c_str())
            );

        // Only show the paste button if the clipboard contains something.
        if (s_Clipboard.Action != ClipboardAction_None)
        {
            if (ImGui::Button("Paste", buttonSize))
            {
                Paste();
                ImGui::CloseCurrentPopup();
            }
        }

        // Only show the copy DVD button if the DVD is available.
        if (m_IsDvdAvailable)
        {
            if (ImGui::Button("Copy DVD", buttonSize))
            {
                CopyDvd();
                ImGui::CloseCurrentPopup();
            }
        }

        // If the directory creation was successful, close this popup.
        if (directoryCreated)
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

void DeviceExplorer::RenderActionBar()
{
    ImGui::BeginChild("Action bar");

    auto endWindowGuard = MakeScopeGuard([]() {
        ImGui::EndChild();
    });

    bool hasSelection = !m_Files.empty() && m_SelectedFileIndex < m_Files.size();
    bool isDir = hasSelection && (m_Files[m_SelectedFileIndex].Attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    bool isXex = hasSelection && m_Files[m_SelectedFileIndex].FullPath.Extension() == ".xex";

    // Build the list of contextual hints based on the currently selected file.
    std::vector<std::pair<const char *, const char *>> hints;

    if (isDir)
        hints.emplace_back(std::make_pair(CHAR_BUTTON_A, "Open"));
    else if (isXex)
        hints.emplace_back(std::make_pair(CHAR_BUTTON_A, "Launch"));

    if (!m_CurrentDir.IsRoot())
        hints.emplace_back(std::make_pair(CHAR_BUTTON_B, "Back"));

    if (hasSelection)
        hints.emplace_back(std::make_pair(CHAR_BUTTON_Y, "Options"));

    if (!m_DeviceInfo.ReadOnly)
        hints.emplace_back(std::make_pair(CHAR_BUTTON_X, "Menu"));

    hints.emplace_back(std::make_pair(CHAR_BUTTON_BACK, "Info"));

    if (hints.empty())
        return;

    // Render the hints horizontally with some gap between them.
    for (size_t i = 0; i < hints.size(); i++)
    {
        if (i != 0)
            ImGui::SameLine(0.0f, 50.0f);

        ImGui::Text("%s  %s", hints[i].first, hints[i].second);
    }

    // Render the current path in the clipboard if there is one.
    if (s_Clipboard.Action != ClipboardAction_None)
    {
        // Create the text.
        const char *actionText = (s_Clipboard.Action == ClipboardAction_Cut) ? "Cut" : "Copy";
        std::string clipboardText = XexUtils::Formatter::Format("%s: %s", actionText, s_Clipboard.Path.String().c_str());

        // Align the text to the right of the window.
        float windowWidth = ImGui::GetWindowContentRegionMax().x;
        float textWidth = ImGui::CalcTextSize(clipboardText.c_str()).x;
        ImGui::SameLine(windowWidth - textWidth);

        // Render the text.
        ImGui::Text("%s", clipboardText.c_str());
    }
}

void DeviceExplorer::RenderProgress()
{
    // Don't render anything when no operations are active.
    if (!m_ActiveOperation)
        return;

    // Open the modal the first time.
    if (!ImGui::IsPopupOpen("Progress"))
        ImGui::OpenPopup("Progress");

    // Render the modal.
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, 0.0f));
    if (ImGui::BeginPopupModal("Progress", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        AsyncFileOperation::Progress progress = m_ActiveOperation->GetProgress();

        // Render the progress bars while the operation is running.
        if (progress.CurrentStatus == AsyncFileOperation::Status_Running)
        {
            // Progress bar for the global progression.
            ImGui::Text("Progress: %i / %i", progress.ProcessedFileCount, progress.TotalFileCount);
            float fileCountFraction = (progress.TotalFileCount > 0) ? static_cast<float>(progress.ProcessedFileCount) / static_cast<float>(progress.TotalFileCount) : 0.0f;
            ImGui::ProgressBar(fileCountFraction);
            ImGui::NewLine();

            // Progress bar for the file currently being processed.
            ImGui::Text(
                "%s: %s / %s",
                progress.CurrentFilePath.Filename().c_str(),
                FormatBytesAsFileSize(progress.CurrentFileBytesTransferred).c_str(),
                FormatBytesAsFileSize(progress.CurrentFileSize).c_str()
            );
            double currentFileFraction = progress.CurrentFileSize > 0 ? static_cast<double>(progress.CurrentFileBytesTransferred) / static_cast<double>(progress.CurrentFileSize) : 0.0f;
            ImGui::ProgressBar(static_cast<float>(currentFileFraction));
            ImGui::NewLine();

            // Align the cancel button to the right.
            ImVec2 buttonSize(ImGui::GetFontSize() * 4.0f, 0.0f);
            float availableWidth = ImGui::GetContentRegionAvail().x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availableWidth - buttonSize.x);
            if (ImGui::Button("Cancel", buttonSize))
                m_ActiveOperation->RequestCancel();
        }
        else
        {
            // Display a notification if the operation failed.
            if (progress.CurrentStatus == AsyncFileOperation::Status_Failed)
                XexUtils::Xam::XNotify(progress.ErrorMessage, XexUtils::Xam::XNOTIFYUI_TYPE_AVOID_REVIEW);

            // Refresh and close the popup.
            RefreshFileList();
            m_ActiveOperation.reset();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void DeviceExplorer::RenderDeviceInfo()
{
    // Open the device info popup if requested.
    if (m_ShouldOpenDeviceInfo)
    {
        ImGui::OpenPopup("Device info");
        m_ShouldOpenDeviceInfo = false;
    }

    // Render the modal.
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, 0.0f));
    if (ImGui::BeginPopupModal("Device info", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        // General info.
        ImGui::Text("Name: %s", m_DeviceInfo.Name.c_str());
        ImGui::Text("Path: %s", m_DeviceInfo.Path.c_str());
        ImGui::Text("Read only: %s", m_DeviceInfo.ReadOnly ? "Yes" : "No");
        ImGui::NewLine();

        // The available space.
        ImGui::Text("Total space: %s", FormatBytesAsFileSize(m_TotalBytes).c_str());
        ImGui::Text("Available space: %s", FormatBytesAsFileSize(m_FreeBytes).c_str());
        double fraction = m_TotalBytes > 0 ? static_cast<double>(m_TotalBytes - m_FreeBytes) / static_cast<double>(m_TotalBytes) : 0;
        ImGui::ProgressBar(static_cast<float>(fraction));

        // Align the close button to the right.
        ImVec2 buttonSize(ImGui::GetFontSize() * 4.0f, 0.0f);
        float availableWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availableWidth - buttonSize.x);
        if (ImGui::Button("Close", buttonSize))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

bool DeviceExplorer::OnButtonPressed(ButtonPressedEvent &event)
{
    const XexUtils::Input::Gamepad &gamepad = event.GetGamepad();

    // If any popup is open, let ImGui handle the button press. For example, we don't
    // want to go the parent directory when pressing B to close a popup.
    if (ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
        return true;

    // Go to the parent directory when pressing B.
    if (gamepad.PressedButtons & XINPUT_GAMEPAD_B)
    {
        if (!m_CurrentDir.IsRoot())
        {
            ChangeDir(m_CurrentDir.Parent());
            return true;
        }
    }

    // Open the options when pressing Y.
    if (gamepad.PressedButtons & XINPUT_GAMEPAD_Y)
    {
        m_ShouldOpenOptions = true;
        return true;
    }

    // Open the menu when pressing X (the menu is not available for read only devices).
    if (gamepad.PressedButtons & XINPUT_GAMEPAD_X && !m_DeviceInfo.ReadOnly)
    {
        m_ShouldOpenMenu = true;
        return true;
    }

    // Open the device info modal when pressing back.
    if (gamepad.PressedButtons & XINPUT_GAMEPAD_BACK)
    {
        m_ShouldOpenDeviceInfo = true;
        return true;
    }

    return false;
}

void DeviceExplorer::ChangeDir(const XexUtils::Fs::Path &dirPath)
{
    // Set the state.
    m_CurrentDir = dirPath;
    m_SelectedFileIndex = 0;
    m_ErrorMessage.clear();
    m_ShouldFocusFirstItem = true;

    // List the files.
    auto newFiles = XexUtils::Fs::ReadDirectory(dirPath);
    if (!newFiles)
    {
        m_ErrorMessage = XexUtils::Formatter::Format(
            "Couldn't read the files in %s. The directory may have been deleted.",
            dirPath.c_str()
        );
        return;
    }

    // Save the list of files.
    m_Files = std::move(*newFiles);
}

void DeviceExplorer::CreateDir(const XexUtils::Fs::Path &dirPath)
{
    BOOL success = CreateDirectory(dirPath.c_str(), nullptr);
    if (!success)
    {
        uint32_t error = GetLastError();
        XexUtils::Fs::Path &dirName = dirPath.Filename();

        if (error == ERROR_ALREADY_EXISTS)
            throw Exception("[DeviceExplorer]: A directory called %s already exists.", dirName.c_str());

        throw Exception("[DeviceExplorer]: Couldn't create the %s directory (%i).", dirName.c_str(), error);
    }
}

void DeviceExplorer::DeleteFile(const XexUtils::Fs::Path &filePath)
{
    BOOL success = ::DeleteFile(filePath.c_str());
    if (!success)
    {
        uint32_t error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND)
            throw Exception("[DeviceExplorer]: File not found.");

        if (error == ERROR_ACCESS_DENIED)
            throw Exception("[DeviceExplorer]: Access denied");

        throw Exception("[DeviceExplorer]: Couldn't delete %s (%i).", filePath.Filename().c_str(), error);
    }
}

void DeviceExplorer::DeleteDir(const XexUtils::Fs::Path &dirPath)
{
    // List the files in the directory.
    auto files = XexUtils::Fs::ReadDirectory(dirPath);
    if (!files)
        throw Exception("[DeviceExplorer]: Couldn't read the files in %s.", dirPath.Filename().c_str());

    // Delete every file inside the directory and recursively delete the sub directories.
    for (size_t i = 0; i < files->size(); i++)
    {
        const auto &file = (*files)[i];

        if (file.Attributes & FILE_ATTRIBUTE_DIRECTORY)
            DeleteDir(file.FullPath);
        else
            DeleteFile(file.FullPath);
    }

    // Delete the current directory once it's empty.
    BOOL success = RemoveDirectory(dirPath.c_str());
    if (!success)
    {
        uint32_t error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND)
            throw Exception("[DeviceExplorer]: File not found.");

        if (error == ERROR_ACCESS_DENIED)
            throw Exception("[DeviceExplorer]: Access denied");

        throw Exception("[DeviceExplorer]: Couldn't delete %s (%i).", dirPath.Filename().c_str(), error);
    }
}

void DeviceExplorer::RenameFile(const XexUtils::Fs::Path &oldPath, const XexUtils::Fs::Path &newPath)
{
    BOOL success = MoveFileEx(oldPath.c_str(), newPath.c_str(), MOVEFILE_COPY_ALLOWED);
    if (!success)
    {
        XexUtils::Fs::Path newFilename = newPath.Filename();

        uint32_t error = GetLastError();
        if (error == ERROR_ALREADY_EXISTS)
            throw Exception("[DeviceExplorer]: A file or directory called %s already exists.", newFilename.c_str());

        throw Exception("[DeviceExplorer]: Couldn't rename %s (%i).", newFilename.c_str(), error);
    }
}

void DeviceExplorer::Paste()
{
    XASSERT(s_Clipboard.Action != ClipboardAction_None);

    XexUtils::Fs::Path destinationPath = m_CurrentDir / s_Clipboard.Path.Filename();
    m_ActiveOperation = std::unique_ptr<AsyncFileOperation>(new AsyncFileOperation());

    if (s_Clipboard.Action == ClipboardAction_Copy)
        m_ActiveOperation->Copy(s_Clipboard.Path, destinationPath);
    else if (s_Clipboard.Action == ClipboardAction_Cut)
        m_ActiveOperation->Move(s_Clipboard.Path, destinationPath);

    s_Clipboard.Clear();
}

void DeviceExplorer::CopyDvd()
{
    m_ActiveOperation = std::unique_ptr<AsyncFileOperation>(new AsyncFileOperation());
    m_ActiveOperation->Copy("dvd:\\", m_CurrentDir);
}

void DeviceExplorer::RefreshFileList()
{
    ChangeDir(m_CurrentDir);
}

void DeviceExplorer::Clipboard::Cut(const XexUtils::Fs::Path &path)
{
    s_Clipboard.Action = ClipboardAction_Cut;
    s_Clipboard.Path = path;
}

void DeviceExplorer::Clipboard::Copy(const XexUtils::Fs::Path &path)
{
    s_Clipboard.Action = ClipboardAction_Copy;
    s_Clipboard.Path = path;
}

void DeviceExplorer::Clipboard::Clear()
{
    s_Clipboard.Action = ClipboardAction_None;
    s_Clipboard.Path = "";
}

bool DeviceExplorer::IsDvdAvailable()
{
    // The DeviceWatcher will create a "dvd:" symlink if a DVD is in the disc tray, so
    // we just check if this symlink exists.

    STRING linkName = {};
    OBJECT_ATTRIBUTES linkAttributes = {};
    RtlInitAnsiString(&linkName, "\\??\\dvd:");
    InitializeObjectAttributes(&linkAttributes, &linkName, OBJ_CASE_INSENSITIVE, nullptr);

    HANDLE handle = INVALID_HANDLE_VALUE;
    NTSTATUS status = NtOpenSymbolicLinkObject(&handle, &linkAttributes);
    NtClose(handle);

    return NT_SUCCESS(status);
}

std::string DeviceExplorer::FormatBytesAsFileSize(uint64_t bytes, size_t decimalPlaces)
{
    // Constants for the units.
    const double oneKilobyte = 1024.0;
    const double oneMegabyte = 1024.0 * oneKilobyte;
    const double oneGigabyte = 1024.0 * oneMegabyte;

    const double bytesAsDouble = static_cast<double>(bytes);

    // Find the most appropriate unit based on the amount of bytes.
    double divider = 1.0;
    std::string unit = "B";
    if (bytesAsDouble > oneGigabyte)
    {
        divider = oneGigabyte;
        unit = "GB";
    }
    else if (bytes > oneMegabyte)
    {
        divider = oneMegabyte;
        unit = "MB";
    }
    else if (bytes > oneKilobyte)
    {
        divider = oneKilobyte;
        unit = "KB";
    }

    // Create the final string.
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(decimalPlaces) << (bytesAsDouble / divider);
    stream << " " << unit;

    return stream.str();
}

std::string DeviceExplorer::TimeToString(time_t time)
{
    // Parse the time into its different components.
    tm t = {};
    errno_t error = localtime_s(&t, &time);
    if (error != 0)
        throw Exception("[DeviceExplorer]: Couldn't convert time %lld to a string (%i).", time, error);

    // Stringify the time.
    char buffer[32] = {};
    strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M", &t);

    return buffer;
}
