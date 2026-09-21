#include <XexUtils.h>
#include <imgui.h>
#include <string>
#include <vector>
#include <xtl.h>

#include "../Core/Exceptions.h"
#include "../Core/Scene.h"
#include "../UI/Theme.h"
#include "PluginManager.h"

PluginManager::PluginManager()
{
    // List the XEX files from the plugins directory.
    auto files = XexUtils::Fs::ReadDirectory("hdd:\\Plugins", "*.xex");
    if (!files)
        return;

    // Create the list of plugins based on the files.
    m_Plugins.reserve(files->size());
    for (size_t i = 0; i < files->size(); i++)
    {
        const auto &file = (*files)[i];

        Plugin plugin = {};
        plugin.Name = file.FullPath.Filename().String();
        plugin.Path = file.FullPath;
        plugin.Loaded = GetModuleHandle(plugin.Path.c_str()) != nullptr;
        plugin.NewLoadState = plugin.Loaded;
        m_Plugins.emplace_back(plugin);
    }
}

void PluginManager::Render()
{
    // Render the plugins in the first column.
    ImVec2 listSize(ImGui::GetContentRegionAvail().x * 0.65f, ImGui::GetContentRegionAvail().y);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
    ImGui::BeginChild("Plugin list", listSize, false, ImGuiWindowFlags_NavFlattened | ImGuiWindowFlags_AlwaysUseWindowPadding);
    RenderPluginList();
    ImGui::EndChild();
    ImGui::PopStyleVar();

    // Draw a vertical separator in the gap between the two columns, replicating
    // ImGuiTableFlags_BordersInnerV from a table-based layout.
    ImVec2 listRectMin = ImGui::GetItemRectMin();
    ImVec2 listRectMax = ImGui::GetItemRectMax();
    float separatorX = listRectMax.x + ImGui::GetStyle().ItemSpacing.x * 0.5f;
    ImU32 separatorColor = ImGui::GetColorU32(ImGuiCol_Separator);
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(separatorX, listRectMin.y),
        ImVec2(separatorX, listRectMax.y),
        separatorColor
    );

    // Create a horizontal layout.
    ImGui::SameLine();

    // Render the logs in the second column.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
    ImGui::BeginChild("Plugin logs", ImVec2(), false, ImGuiWindowFlags_AlwaysUseWindowPadding);
    RenderLogs();
    ImGui::EndChild();
    ImGui::PopStyleVar();
}

PluginManager::LogLine::LogLine(const std::string &text, const ImVec4 &color)
    : Text(text), Color(color)
{
}

void PluginManager::RenderPluginList()
{
    // Render a placeholder text if no plugins were found.
    if (m_Plugins.empty())
    {
        ImGui::Text("No plugins found.");
        return;
    }

    // Render a checkbox for each plugin.
    for (size_t i = 0; i < m_Plugins.size(); i++)
        ImGui::Checkbox(m_Plugins[i].Name.c_str(), &m_Plugins[i].NewLoadState);

    ImGui::NewLine();

    // Load/unload the plugins that changed state.
    if (ImGui::Button("Apply changes"))
    {
        UnloadPlugins();
        LoadPlugins();
    }
}

void PluginManager::RenderLogs()
{
    for (size_t i = 0; i < m_Logs.size(); i++)
        ImGui::TextColored(m_Logs[i].Color, m_Logs[i].Text.c_str());
}

void PluginManager::LoadPlugins()
{
    for (size_t i = 0; i < m_Plugins.size(); i++)
    {
        // If the plugin is still in the same state, skip it.
        Plugin &plugin = m_Plugins[i];
        if (plugin.Loaded == plugin.NewLoadState)
            continue;

        // If the plugin doesn't need to be loaded, skip it.
        if (plugin.NewLoadState == false)
            continue;

        // Load the plugin.
        NTSTATUS status = LoadImage(plugin.Path.c_str());
        if (!NT_SUCCESS(status))
        {
            LogError(
                XexUtils::Formatter::Format("Failed to load %s (%X).", plugin.Name.c_str(), status)
            );
            plugin.NewLoadState = false;
            continue;
        }

        plugin.Loaded = true;
        LogSuccess(XexUtils::Formatter::Format("Loaded %s.", plugin.Name.c_str()));
    }
}

void PluginManager::UnloadPlugins()
{
    for (size_t i = 0; i < m_Plugins.size(); i++)
    {
        // If the plugin is still in the same state, skip it.
        Plugin &plugin = m_Plugins[i];
        if (plugin.Loaded == plugin.NewLoadState)
            continue;

        // If the plugin doesn't need to be unloaded, skip it.
        if (plugin.NewLoadState == true)
            continue;

        // If the plugin was unloaded via an external process between when the scene
        // started and now, warn the user and skip.
        HANDLE handle = GetModuleHandle(plugin.Path.c_str());
        if (handle == nullptr)
        {
            LogWarn(
                XexUtils::Formatter::Format("%s was already unloaded.", plugin.Name.c_str())
            );
            continue;
        }

        // Unload the plugin.
        NTSTATUS status = UnloadImage(handle);
        if (!NT_SUCCESS(status))
        {
            LogError(
                XexUtils::Formatter::Format("Failed to unload %s (%X).", plugin.Name.c_str(), status)
            );
            plugin.NewLoadState = true;
            continue;
        }

        plugin.Loaded = false;
        LogSuccess(XexUtils::Formatter::Format("Unloaded %s.", plugin.Name.c_str()));
    }
}

NTSTATUS PluginManager::LoadImage(const char *imageName)
{
    // Start a system thread to load the plugin, plugins are almost always system DLLs and
    // those can only be loaded from system threads.
    HANDLE threadHandle = XexUtils::ThreadEx(
        LoadImageThreadProc,
        static_cast<void *>(const_cast<char *>(imageName)),
        EXCREATETHREAD_FLAG_SYSTEM | EXCREATETHREAD_FLAG_CORE4
    );
    WaitForSingleObject(threadHandle, INFINITE);

    // The thread exit code is the return value of XexLoadImage.
    DWORD exitCode = 0;
    BOOL success = GetExitCodeThread(threadHandle, &exitCode);
    if (!success)
        throw Exception("[PluginManager]: Couldn't get the XexLoadImage return value (%i).", GetLastError());

    return static_cast<NTSTATUS>(exitCode);
}

NTSTATUS PluginManager::UnloadImage(HANDLE handle)
{
    // Start a system thread to unload the plugin, plugins are almost always system DLLs and
    // those can only be unloaded from system threads.
    HANDLE threadHandle = XexUtils::ThreadEx(
        UnloadImageThreadProc,
        handle,
        EXCREATETHREAD_FLAG_SYSTEM | EXCREATETHREAD_FLAG_CORE4
    );
    WaitForSingleObject(threadHandle, INFINITE);

    // The thread exit code is the return value of XexUnloadImage.
    DWORD exitCode = 0;
    BOOL success = GetExitCodeThread(threadHandle, &exitCode);
    if (!success)
        throw Exception("[PluginManager]: Couldn't get the XexUnloadImage return value (%i).", GetLastError());

    return static_cast<NTSTATUS>(exitCode);
}

void PluginManager::LogSuccess(const std::string &text)
{
    m_Logs.emplace_back(LogLine(text, Theme::Colors::Green));
}

void PluginManager::LogError(const std::string &text)
{
    m_Logs.emplace_back(LogLine(text, Theme::Colors::Red));
}

void PluginManager::LogWarn(const std::string &text)
{
    m_Logs.emplace_back(LogLine(text, Theme::Colors::Yellow));
}

DWORD WINAPI PluginManager::LoadImageThreadProc(void *pArgs)
{
    const char *imageName = static_cast<const char *>(pArgs);

    return XexLoadImage(imageName, XEX_LOADING_FLAG_DLL, 0, nullptr);
}

DWORD WINAPI PluginManager::UnloadImageThreadProc(void *pArgs)
{
    HANDLE handle = static_cast<HANDLE>(pArgs);

    // The plugin load count is normally set to 0xFFFF (I don't know why) so we need
    // to set it to 1 right before unloading to make XexUnloadImage actually unmap the
    // plugin from memory.
    LDR_DATA_TABLE_ENTRY *pDataTable = static_cast<LDR_DATA_TABLE_ENTRY *>(handle);
    pDataTable->LoadCount = 1;

    return XexUnloadImage(handle);
}
