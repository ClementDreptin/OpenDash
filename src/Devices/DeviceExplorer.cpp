#include <XexUtils.h>
#include <cstdint>
#include <imgui.h>
#include <string>
#include <utility>
#include <vector>
#include <xtl.h>

#include "../Core/Exceptions.h"
#include "../Core/Scene.h"
#include "../Input/InputWatcher.h"
#include "../Renderer/Renderer.h"
#include "../Utils/ScopeGuard.h"
#include "DeviceExplorer.h"

DeviceExplorer::Clipboard DeviceExplorer::s_Clipboard;

DeviceExplorer::DeviceExplorer(const XexUtils::Fs::Path &baseDir)
    : m_SelectedFileIndex(0),
      m_DirectoryTexture("game:\\assets\\images\\directory.png"),
      m_FileTexture("game:\\assets\\images\\file.png"),
      m_XexTexture("game:\\assets\\images\\xex.png"),
      m_ShouldFocusFirstItem(false),
      m_ShouldOpenMenu(false),
      m_ShouldOpenOptions(false)
{
    ChangeDir(baseDir);
}

void DeviceExplorer::Render()
{
    RenderFileList();

    RenderOptions();

    RenderMenu();

    RenderActionBar();
}

void DeviceExplorer::OnEvent(Event &event)
{
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<ButtonPressedEvent>([this](ButtonPressedEvent &e) { return OnButtonPressed(e); });
}

void DeviceExplorer::RenderFileList()
{
    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NavFlattened |
        ImGuiWindowFlags_AlwaysUseWindowPadding;

    // Setup a child window with extra padding for the alignment.
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12.0f, 12.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
    float listHeight = ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y;
    ImGui::BeginChild("File list", ImVec2(0.0f, -listHeight), false, windowFlags);

    // Automatically end this window when this scope ends.
    auto endWindowGuard = MakeScopeGuard([]() {
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
    });

    // State to keep across renders.
    static bool changeDirectoryRequested = false;

    // Render the error message if there is one.
    if (!m_ErrorMessage.empty())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.39f, 0.40f, 1.0f), m_ErrorMessage.c_str());
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
        bool isXex = file.Name.Extension() == ".xex";
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
        std::string label = "##" + file.Name.String();
        if (ImGui::Selectable(label.c_str(), m_SelectedFileIndex == i, 0, ImVec2(0.0f, texture.GetHeight())))
        {
            if (isDir)
                changeDirectoryRequested = true;
            else if (isXex)
                XLaunchNewImage((m_CurrentDir / file.Name).c_str(), 0);
        }

        if (ImGui::IsItemFocused())
            m_SelectedFileIndex = i;

        // Move the cursor back to where it was before creating the selectable so that
        // the next thing we push is at the beginning of the selectable.
        ImGui::SetCursorPos(cursorPos);

        // Render the appropriate icon.
        ImGui::Image(texture.GetHandle(), ImVec2(texture.GetWidth(), texture.GetHeight()));
        ImGui::SameLine();

        // Vertically align the text with the middle of the icon.
        ImGui::SetCursorPosY(cursorPos.y + (texture.GetHeight() - ImGui::GetTextLineHeight()) * 0.5f);
        ImGui::Text(file.Name.c_str());
    }

    // Change directory if requested.
    if (changeDirectoryRequested)
    {
        changeDirectoryRequested = false;
        ChangeDir(m_CurrentDir / m_Files[m_SelectedFileIndex].Name);
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
    bool shouldOpenConfirm = false;

    // Update the keyboard while it's open.
    if (m_Keyboard.GetState() == NativeKeyboard::State_Pending)
        m_Keyboard.Update();

    bool fileRenamed = false;
    if (m_Keyboard.GetState() == NativeKeyboard::State_Success)
    {
        // Reset the state of the keyboard so that this if only runs once.
        m_Keyboard.Reset();

        try
        {
            // Rename the file.
            XexUtils::Fs::Path oldPath = m_CurrentDir / file.Name;
            XexUtils::Fs::Path newPath = m_CurrentDir / m_Keyboard.GetResult();
            Move(oldPath, newPath);
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

        // The delete button only opens the confirm modal.
        if (ImGui::Button("Delete", buttonSize))
            shouldOpenConfirm = true;

        if (ImGui::Button("Cut", buttonSize))
        {
            s_Clipboard.Cut(m_CurrentDir / file.Name);
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::Button("Copy", buttonSize))
        {
            s_Clipboard.Copy(m_CurrentDir / file.Name);
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::Button("Rename", buttonSize))
            m_Keyboard.Show(
                "Rename",
                XexUtils::Formatter::Format("Rename %s.", file.Name.c_str()),
                file.Name.c_str()
            );

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
        ImGui::Text("Are you sure you want to delete %s?", file.Name.c_str());
        ImGui::NewLine();

        // Align the buttons to the right of the modal.
        ImVec2 buttonSize(ImGui::GetFontSize() * 4.0f, 0.0f);
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float groupWidth = buttonSize.x * 2.0f + spacing;
        float availWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availWidth - groupWidth);

        if (ImGui::Button("Yes", buttonSize))
        {
            try
            {
                // Delete the file or directory.
                XexUtils::Fs::Path fullPath = m_CurrentDir / file.Name;
                if (file.Attributes & FILE_ATTRIBUTE_DIRECTORY)
                    DeleteDir(fullPath);
                else
                    DeleteFile(fullPath);

                RefreshFileList();
                ImGui::CloseCurrentPopup();
            }
            catch (const std::exception &exception)
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
    if (m_Keyboard.GetState() == NativeKeyboard::State_Pending)
        m_Keyboard.Update();

    bool directoryCreated = false;
    if (m_Keyboard.GetState() == NativeKeyboard::State_Success)
    {
        // Reset the state of the keyboard so that this if only runs once.
        m_Keyboard.Reset();

        try
        {
            // Change directory.
            XexUtils::Fs::Path newDir = m_CurrentDir / m_Keyboard.GetResult();
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
            m_Keyboard.Show(
                "Create directory",
                XexUtils::Formatter::Format("Create a directory in %s.", m_CurrentDir.c_str())
            );

        // Only show the paste button if the clipboard contains something.
        if (s_Clipboard.Action != ClipboardAction_None)
        {
            if (ImGui::Button("Paste", buttonSize))
            {
                try
                {
                    Paste();
                    RefreshFileList();
                    ImGui::CloseCurrentPopup();
                }
                catch (const std::exception &exception)
                {
                    XexUtils::Xam::XNotify(exception.what(), XexUtils::Xam::XNOTIFYUI_TYPE_AVOID_REVIEW);
                }
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
    bool isXex = hasSelection && m_Files[m_SelectedFileIndex].Name.Extension() == ".xex";

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

    hints.emplace_back(std::make_pair(CHAR_BUTTON_BACK, "Menu"));

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

    // Open the menu when pressing back.
    if (gamepad.PressedButtons & XINPUT_GAMEPAD_BACK)
    {
        m_ShouldOpenMenu = true;
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
        XexUtils::Fs::Path fullPath = dirPath / file.Name;

        if (file.Attributes & FILE_ATTRIBUTE_DIRECTORY)
            DeleteDir(fullPath);
        else
            DeleteFile(fullPath);
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

void DeviceExplorer::Move(const XexUtils::Fs::Path &oldPath, const XexUtils::Fs::Path &newPath)
{
    // MoveFileEx from Win32 won't move directories across devices, even if the
    // MOVEFILE_COPY_ALLOWED flag is passed, so a manual move of each file is required.
    bool oldPathIsDir = (GetFileAttributes(oldPath.c_str()) & FILE_ATTRIBUTE_DIRECTORY) != 0;
    bool newPathIsOnDifferentDevice = oldPath.Drive() != newPath.Drive();
    if (oldPathIsDir && newPathIsOnDifferentDevice)
    {
        MoveDirAcrossDevices(oldPath, newPath);
        return;
    }

    // When copying files/directories on the same device, or regular files across devices,
    // MoveFileEx can handle it directly.
    BOOL success = MoveFileEx(oldPath.c_str(), newPath.c_str(), MOVEFILE_COPY_ALLOWED);
    if (!success)
    {
        XexUtils::Fs::Path newFilename = newPath.Filename();

        uint32_t error = GetLastError();
        if (error == ERROR_ALREADY_EXISTS)
            throw Exception("[DeviceExplorer]: A file or directory called %s already exists.", newFilename.c_str());

        throw Exception("[DeviceExplorer]: Couldn't move %s (%i).", newFilename.c_str(), error);
    }
}

void DeviceExplorer::MoveDirAcrossDevices(const XexUtils::Fs::Path &oldPath, const XexUtils::Fs::Path &newPath)
{
    // Create the new directory.
    BOOL success = CreateDirectory(newPath.c_str(), nullptr);
    if (!success)
    {
        uint32_t error = GetLastError();

        // It's fine if the new directory already exists, this allows merging the old
        // directory into the new one.
        if (error != ERROR_ALREADY_EXISTS)
            throw Exception("[DeviceExplorer]: Couldn't create directory %s (%i).", newPath.Filename().c_str(), error);
    }

    // List the files to move.
    auto files = XexUtils::Fs::ReadDirectory(oldPath);
    if (!files)
        throw Exception("[DeviceExplorer]: Couldn't read the files in %s.", oldPath.Filename().c_str());

    // Move every file from the old directory to the new one and recursively move the sub directories.
    for (size_t i = 0; i < files->size(); i++)
    {
        const auto &file = (*files)[i];

        if (file.Attributes & FILE_ATTRIBUTE_DIRECTORY)
            MoveDirAcrossDevices(oldPath / file.Name, newPath / file.Name);
        else
            Move(oldPath / file.Name, newPath / file.Name);
    }

    // Delete the now empty old directory.
    DeleteDir(oldPath);
}

void DeviceExplorer::CopyFile(const XexUtils::Fs::Path &oldPath, const XexUtils::Fs::Path &newPath)
{
    BOOL success = ::CopyFile(oldPath.c_str(), newPath.c_str(), FALSE);
    if (!success)
    {
        XexUtils::Fs::Path newFilename = newPath.Filename();

        uint32_t error = GetLastError();
        throw Exception("[DeviceExplorer]: Couldn't copy %s (%i).", newFilename.c_str(), error);
    }
}

void DeviceExplorer::CopyDir(const XexUtils::Fs::Path &oldPath, const XexUtils::Fs::Path &newPath)
{
    // Create the new directory.
    BOOL success = CreateDirectory(newPath.c_str(), nullptr);
    if (!success)
    {
        uint32_t error = GetLastError();

        // It's fine if the new directory already exists, this allows merging the old
        // directory into the new one.
        if (error != ERROR_ALREADY_EXISTS)
            throw Exception("[DeviceExplorer]: Couldn't create directory %s (%i).", newPath.Filename().c_str(), error);
    }

    // List the files to move.
    auto files = XexUtils::Fs::ReadDirectory(oldPath);
    if (!files)
        throw Exception("[DeviceExplorer]: Couldn't read the files in %s.", oldPath.Filename().c_str());

    // Copy every file from the old directory to the new one and recursively copy the sub directories.
    for (size_t i = 0; i < files->size(); i++)
    {
        const auto &file = (*files)[i];

        if (file.Attributes & FILE_ATTRIBUTE_DIRECTORY)
            CopyDir(oldPath / file.Name, newPath / file.Name);
        else
            CopyFile(oldPath / file.Name, newPath / file.Name);
    }
}

void DeviceExplorer::Paste()
{
    XASSERT(s_Clipboard.Action != ClipboardAction_None);

    XexUtils::Fs::Path destinationPath = m_CurrentDir / s_Clipboard.Path.Filename();

    // Copy.
    if (s_Clipboard.Action == ClipboardAction_Copy)
    {
        // Check if the copied path is a directory.
        FILE_ATTRIBUTE clipboardPathAttributes = GetFileAttributes(s_Clipboard.Path.c_str());
        bool clipboardPathIsDir = clipboardPathAttributes != 0xFFFFFFFF && (clipboardPathAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        if (clipboardPathIsDir)
            CopyDir(s_Clipboard.Path, destinationPath);
        else
            CopyFile(s_Clipboard.Path, destinationPath);
    }
    // Cut.
    else if (s_Clipboard.Action == ClipboardAction_Cut)
        Move(s_Clipboard.Path, destinationPath);

    s_Clipboard.Clear();
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
