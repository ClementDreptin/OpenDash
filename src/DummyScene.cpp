#include <imgui.h>

#include "DummyScene.h"
#include "Renderer.h"

void DummyScene::Render()
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
    ImGui::Begin("Dummy Scene", nullptr, windowFlags);

    ImGui::Text("first");
    ImGui::Text("second");
    ImGui::Text("third");

    ImGui::End();
    ImGui::PopStyleVar(3);
}
