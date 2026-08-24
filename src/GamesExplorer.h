#pragma once

#include <xtl.h>
#include <vector>
#include <string>
#include <imgui.h>
#include <XexUtils.h>

#include "Renderer.h"

class GamesExplorer
{
public:
    GamesExplorer();

    ~GamesExplorer();

    void Render();

private:
    struct Game
    {
        Game();

        XexUtils::Fs::Path DirPath;
        std::string Name;
        uint32_t TitleId;
        XexUtils::Optional<Texture> IconTexture;
        XexUtils::Optional<Texture> BackgroundTexture;
    };

    std::vector<Game> m_Games;
    size_t m_SelectedGameIndex;
    HANDLE m_ScanThread;
    CRITICAL_SECTION m_GamesLock;
    bool m_Scanning;

    static const ImVec2 s_IconSize;
    static const ImVec2 s_BackgroundSize;

private:
    static DWORD WINAPI ScanGamesThreadProc(void *pArgs);

    void RenderGameList();

    void RenderCurrentGameInfo();

    void LaunchGame(const Game &game);

    void EnrichGameFromNxeart(Game &game);
};
