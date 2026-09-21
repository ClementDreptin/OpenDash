#pragma once

#include <string>
#include <vector>
#include <xtl.h>

#include "../Core/Scene.h"

class PluginManager : public Scene
{
public:
    PluginManager();

    void Render() override;

private:
    struct Plugin
    {
        std::string Name;
        XexUtils::Fs::Path Path;
        bool Loaded;
        bool NewLoadState;
    };

    struct LogLine
    {
        LogLine(const std::string &text, const ImVec4 &color);

        std::string Text;
        ImVec4 Color;
    };

    std::vector<Plugin> m_Plugins;
    std::vector<LogLine> m_Logs;

    void RenderPluginList();

    void RenderLogs();

    void LoadPlugins();

    void UnloadPlugins();

    NTSTATUS LoadImage(const char *imageName);

    NTSTATUS UnloadImage(HANDLE handle);

    void LogSuccess(const std::string &text);

    void LogError(const std::string &text);

    void LogWarn(const std::string &text);

private:
    static DWORD WINAPI LoadImageThreadProc(void *pArgs);

    static DWORD WINAPI UnloadImageThreadProc(void *pArgs);
};
