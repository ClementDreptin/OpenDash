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

DeviceExplorer::DeviceExplorer(const XexUtils::Fs::Path &baseDir)
    : m_SelectedFileIndex(0),
      m_DirectoryTexture("game:\\assets\\images\\directory.png"),
      m_FileTexture("game:\\assets\\images\\file.png"),
      m_XexTexture("game:\\assets\\images\\xex.png"),
      m_ShouldFocusFirstItem(false),
      m_ShouldOpenOptions(false)
{
    ChangeDir(baseDir);
}

void DeviceExplorer::Render()
{
    RenderFileList();

    RenderOptions();

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
    // Open the options popup if requested.
    if (m_ShouldOpenOptions)
    {
        ImGui::OpenPopup("Options");
        m_ShouldOpenOptions = false;
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
            directoryCreated = true;

            // Refresh the file list.
            ChangeDir(m_CurrentDir);
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
    if (ImGui::BeginPopup("Options"))
    {
        if (ImGui::Button("Create directory"))
            m_Keyboard.Show(
                "Create directory",
                XexUtils::Formatter::Format("Create a directory in %s.", m_CurrentDir.c_str())
            );

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

    hints.emplace_back(std::make_pair(CHAR_BUTTON_Y, "Options"));

    if (hints.empty())
        return;

    // Render the hints horizontally with some gap between them.
    for (size_t i = 0; i < hints.size(); i++)
    {
        if (i != 0)
            ImGui::SameLine(0.0f, 50.0f);

        ImGui::Text("%s  %s", hints[i].first, hints[i].second);
    }
}

bool DeviceExplorer::OnButtonPressed(ButtonPressedEvent &event)
{
    const XexUtils::Input::Gamepad &gamepad = event.GetGamepad();

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

    return false;
}

void DeviceExplorer::ChangeDir(const XexUtils::Fs::Path &newDir)
{
    // Set the state.
    m_CurrentDir = newDir;
    m_SelectedFileIndex = 0;
    m_ErrorMessage.clear();
    m_ShouldFocusFirstItem = true;

    // List the files.
    auto newFiles = XexUtils::Fs::ReadDirectory(newDir);
    if (!newFiles)
    {
        m_ErrorMessage = XexUtils::Formatter::Format(
            "Couldn't read the files in %s. The directory may have been deleted.",
            newDir.c_str()
        );
        return;
    }

    // Save the list of files.
    m_Files = std::move(*newFiles);
}

void DeviceExplorer::CreateDir(const XexUtils::Fs::Path &newDir)
{
    BOOL success = CreateDirectory(newDir.c_str(), nullptr);
    if (!success)
    {
        uint32_t error = GetLastError();
        XexUtils::Fs::Path &dirName = newDir.Filename();

        if (error == ERROR_ALREADY_EXISTS)
            throw Exception("[DeviceExplorer]: A directory called %s already exists.", dirName.c_str());

        throw Exception("[DeviceExplorer]: Couldn't create the %s directory (%i).", dirName.c_str(), error);
    }
}
