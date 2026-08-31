#include <xtl.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <XexUtils.h>

#include "Exceptions.h"
#include "GamesExplorer.h"
#include "Renderer.h"

// NOTE:
// Right now, the background texture of each game is loaded ahead of time during the scan.
// This takes a significant amount of time (multiple seconds). If this ever becomes a
// problem, it would be better to lazy load the background textures as we scroll through
// the games.

const ImVec2 GamesExplorer::s_IconSize(42.0f, 32.0f);
const ImVec2 GamesExplorer::s_BackgroundSize(323.0f, 182.0f);

GamesExplorer::GamesExplorer()
    : m_SelectedGameIndex(0), m_Scanning(true)
{
    // Initiliaze the scanning lock.
    InitializeCriticalSection(&m_GamesLock);

    // Start a thread that will retrieve the games on the hard drive.
    m_ScanThread = XexUtils::Thread(ScanGamesThreadProc, this);
    if (!m_ScanThread)
    {
        DeleteCriticalSection(&m_GamesLock);
        uint32_t error = GetLastError();
        throw Exception("[GamesExplorer]: Couldn't create the scan thread (%X).", error);
    }
}

GamesExplorer::~GamesExplorer()
{
    // Wait for the scanning thread to finish if needed.
    if (m_ScanThread)
    {
        WaitForSingleObject(m_ScanThread, INFINITE);
        CloseHandle(m_ScanThread);
    }

    DeleteCriticalSection(&m_GamesLock);
}

void GamesExplorer::Render()
{
    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove;

    // Start a window that takes up the full safe area.
    Renderer::Area safeArea = Renderer::GetSafeArea();
    ImGui::SetNextWindowPos(ImVec2(safeArea.Origin.x, safeArea.Origin.y));
    ImGui::SetNextWindowSize(ImVec2(safeArea.Width, safeArea.Height));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14.0f, 10.0f));
    ImGui::Begin("Games Explorer", nullptr, windowFlags);

    // Retrieve the scanning state using the lock.
    bool scanning = false;
    EnterCriticalSection(&m_GamesLock);
    scanning = m_Scanning;
    LeaveCriticalSection(&m_GamesLock);

    // Render the games if we're done scanning.
    if (!scanning)
    {
        // Create a two-column layout using a table.
        if (ImGui::BeginTable("Games table", 2, ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("Game list column", ImGuiTableColumnFlags_WidthStretch, 0.7f);
            ImGui::TableSetupColumn("Game info column", ImGuiTableColumnFlags_WidthStretch, 0.3f);

            ImGui::TableNextRow();

            // The game list is the first column.
            ImGui::TableSetColumnIndex(0);
            RenderGameList();

            // The current game info is the second column.
            ImGui::TableSetColumnIndex(1);
            RenderCurrentGameInfo();

            ImGui::EndTable();
        }
    }
    // Render a loading state otherwise.
    else
        ImGui::Text("Loading games...");

    ImGui::End();
    ImGui::PopStyleVar(3);
}

DWORD WINAPI GamesExplorer::ScanGamesThreadProc(void *pArgs)
{
    GamesExplorer *This = static_cast<GamesExplorer *>(pArgs);

    // List the files in hdd:\Games.
    XexUtils::Fs::Path baseDir = "hdd:\\Games";
    auto files = XexUtils::Fs::ReadDirectory(baseDir);
    if (!files)
    {
        EnterCriticalSection(&This->m_GamesLock);
        This->m_Scanning = false;
        LeaveCriticalSection(&This->m_GamesLock);

        return 0;
    }

    // Build the list of games.
    std::vector<Game> games;
    for (size_t i = 0; i < files->size(); i++)
    {
        const auto &file = (*files)[i];

        // We are looking for directories...
        if (!(file.Attributes & FILE_ATTRIBUTE_DIRECTORY))
            continue;

        // ...that contain a file named default.xex.
        XexUtils::Fs::Path gameDirPath = baseDir / file.Name;
        XexUtils::Fs::Path defaultXexPath = gameDirPath / "default.xex";
        bool hasDefaultXex = GetFileAttributes(defaultXexPath.c_str()) == FILE_ATTRIBUTE_NORMAL;
        if (!hasDefaultXex)
            continue;

        // Create the Game object.
        Game game;
        game.DirPath = gameDirPath;
        game.Name = file.Name.String();
        This->EnrichGameFromNxeart(game);

        // Push the Game object into the vector.
        games.emplace_back(std::move(game));
    }

    // Populate the games and tell the render thread we're done scanning.
    EnterCriticalSection(&This->m_GamesLock);
    This->m_Games = std::move(games);
    This->m_Scanning = false;
    LeaveCriticalSection(&This->m_GamesLock);

    return 0;
}

void GamesExplorer::RenderGameList()
{
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NavFlattened |
        ImGuiWindowFlags_AlwaysUseWindowPadding;

    float listHeight = ImGui::GetContentRegionAvail().y - ImGui::GetStyle().CellPadding.y * 2;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14.0f, 14.0f));
    ImGui::BeginChild("Game list", ImVec2(0.0f, listHeight), false, flags);

    // State to keep across renders.
    static bool showPopup = false;

    // Render the list of games.
    if (!m_Games.empty())
    {
        for (size_t i = 0; i < m_Games.size(); i++)
        {
            // Create a selectable with an icon in it.

            const Game &game = m_Games[i];

            // Save the cursor position before creating the selectable.
            ImVec2 cursorPos = ImGui::GetCursorPos();

            // Create a selectable with an invisible text. It's invisible because it
            // starts with "##".
            std::string label = "##" + game.Name;
            if (ImGui::Selectable(label.c_str(), m_SelectedGameIndex == i, 0, ImVec2(0.0f, s_IconSize.y)))
                showPopup = true;

            if (ImGui::IsItemFocused())
                m_SelectedGameIndex = i;

            // Move the cursor back to where it was before creating the selectable so that
            // the next thing we push is at the beginning of the selectable.
            ImGui::SetCursorPos(cursorPos);

            // Add the icon if it's available.
            if (game.IconTexture)
            {
                ImGui::Image(game.IconTexture->GetHandle(), s_IconSize);
                ImGui::SameLine();
            }

            // Vertically align the text with the middle of the icon.
            ImGui::SetCursorPosY(cursorPos.y + (s_IconSize.y - ImGui::GetTextLineHeight()) * 0.5f);
            ImGui::Text(game.Name.c_str());
        }

        // Setup proper wrapping in the list.
        ImGui::NavMoveRequestTryWrapping(ImGui::GetCurrentWindow(), ImGuiNavMoveFlags_LoopY);
    }
    // If no games were found, just render a placeholder text.
    else
        ImGui::Text("No games found.");

    // Show the popup if requested.
    if (showPopup)
    {
        ImGui::OpenPopup("Game options");
        showPopup = false;
    }

    // The popup content.
    if (ImGui::BeginPopup("Game options"))
    {
        // Launch the game.
        if (ImGui::Selectable("Launch"))
        {
            LaunchGame(m_Games[m_SelectedGameIndex]);
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
}

void GamesExplorer::RenderCurrentGameInfo()
{
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_AlwaysUseWindowPadding;

    float gameInfoHeight = ImGui::GetContentRegionAvail().y - ImGui::GetStyle().CellPadding.y * 2;
    ImGui::BeginChild("Game info", ImVec2(0.0f, gameInfoHeight), false, flags);

    if (!m_Games.empty())
    {
        const Game &game = m_Games[m_SelectedGameIndex];

        // Display some general info about the game.
        ImGui::Text("Directory:");
        ImGui::Text("%s", game.DirPath.c_str());
        ImGui::NewLine();
        ImGui::Text("Title ID:");
        ImGui::Text("%X", game.TitleId);

        // Display the background if it's available.
        if (game.BackgroundTexture)
        {
            ImGui::NewLine();
            ImGui::Text("Background:");
            ImGui::Image(game.BackgroundTexture->GetHandle(), s_BackgroundSize);
        }
    }

    ImGui::EndChild();
}

void GamesExplorer::LaunchGame(const Game &game)
{
    XexUtils::Fs::Path defaultXexPath = game.DirPath / "default.xex";

    XLaunchNewImage(defaultXexPath.c_str(), 0);
}

void GamesExplorer::EnrichGameFromNxeart(Game &game)
{
    XASSERT(!game.DirPath.IsEmpty());

    // Check if the game ships with an nxeart file (all official games should).
    XexUtils::Fs::Path nxeartPath = game.DirPath / "nxeart";
    bool hasNxeart = GetFileAttributes(nxeartPath.c_str()) == FILE_ATTRIBUTE_NORMAL;
    if (!hasNxeart)
    {
        DebugPrint(
            "[GamesExplorer]: Warn: The game at %s doesn't contain an nxeart package.",
            game.DirPath.c_str()
        );
        return;
    }

    XexUtils::StfsPackage nxeart(nxeartPath);
    auto nxeartMetadata = nxeart.ReadMetadata();
    if (!nxeartMetadata)
        return;

    // Get the game's name.
    game.Name = XexUtils::Formatter::ToNarrow(nxeartMetadata->TitleName);

    // Get the game's title ID.
    game.TitleId = nxeartMetadata->ExecutionId.TitleId;

    // Mount the content of the nxeart package.
    std::string titleIdAsHexString = XexUtils::Formatter::Format("%X", game.TitleId);
    HRESULT hr = nxeart.Mount(titleIdAsHexString);
    if (FAILED(hr))
        return;

    // Make sure nxeslot.jpg is present.
    XexUtils::Fs::Path nxeSlotPath = titleIdAsHexString + ":\\nxeslot.jpg";
    bool hasNxeSlot = GetFileAttributes(nxeSlotPath.c_str()) == FILE_ATTRIBUTE_NORMAL;
    if (!hasNxeSlot)
    {
        DebugPrint(
            "[GamesExplorer]: Error: The nxeart package of %s exists but doesn't "
            "contain an nxeslot.jpg file.",
            game.DirPath.c_str()
        );
        return;
    }

    // Create the icon texture from nxeslot.jpg file.
    try
    {
        game.IconTexture = Texture(nxeSlotPath.c_str(), s_IconSize.x, s_IconSize.y);
    }
    catch (const Exception &exception)
    {
        DebugPrint(exception.what());
        return;
    }

    // Make sure nxebg.jpg is present.
    XexUtils::Fs::Path nxeBgPath = titleIdAsHexString + ":\\nxebg.jpg";
    bool hasNxeBg = GetFileAttributes(nxeBgPath.c_str()) == FILE_ATTRIBUTE_NORMAL;
    if (!hasNxeBg)
    {
        DebugPrint(
            "[GamesExplorer]: Error: The nxeart package of %s exists but doesn't "
            "contain an nxebg.jpg file.",
            game.DirPath.c_str()
        );
        return;
    }

    // Create the background texture from nxebg.jpg file.
    try
    {
        game.BackgroundTexture = Texture(nxeBgPath.c_str(), s_BackgroundSize.x, s_BackgroundSize.y);
    }
    catch (const Exception &exception)
    {
        DebugPrint(exception.what());
        return;
    }
}

GamesExplorer::Game::Game()
    : TitleId(0)
{
}
