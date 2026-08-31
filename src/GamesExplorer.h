#pragma once

#include <xtl.h>
#include <vector>
#include <string>
#include <imgui.h>
#include <XexUtils.h>

#include "Renderer.h"
#include "Scene.h"

class GamesExplorer : public Scene
{
public:
    GamesExplorer();

    ~GamesExplorer();

    void Render() override;

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

    void RenderGameList();

    void RenderCurrentGameInfo();

    void EnrichGameFromNxeart(Game &game);

private:
    static const ImVec2 s_IconSize;
    static const ImVec2 s_BackgroundSize;

    static DWORD WINAPI ScanGamesThreadProc(void *pArgs);
};
