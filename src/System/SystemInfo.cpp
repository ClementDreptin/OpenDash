#include <XexUtils.h>
#include <imgui.h>
#include <xtl.h>

#include "../Core/Scene.h"
#include "../Utils/ScopeGuard.h"
#include "SystemInfo.h"

SystemInfo::SystemInfo()
    : m_CpuTemperature(0.0f),
      m_GpuTemperature(0.0f),
      m_RamTemperature(0.0f),
      m_BoardTemperature(0.0f),
      m_LastTemperatureUpdateTickCount(0)
{
    char buffer[16] = {};
    XNADDR address = {};

    // Get the debug IP address.
    XNetGetDebugXnAddr(&address);
    XNetInAddrToString(address.ina, buffer, sizeof(buffer));
    m_DebugIpAddress = buffer;

    ZeroMemory(buffer, sizeof(buffer));
    ZeroMemory(&address, sizeof(address));

    // Get the title IP address.
    XNetGetTitleXnAddr(&address);
    XNetInAddrToString(address.ina, buffer, sizeof(buffer));
    m_TitleIpAddress = buffer;
}

void SystemInfo::Render()
{
    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NavFlattened |
        ImGuiWindowFlags_AlwaysUseWindowPadding;

    // Setup a child window with extra padding for the alignment.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
    ImGui::BeginChild("System Info", ImVec2(), false, windowFlags);

    // Automatically end this window when this scope ends.
    auto endWindowGuard = MakeScopeGuard([]() {
        ImGui::EndChild();
        ImGui::PopStyleVar();
    });

    if (ImGui::BeginTable("System Info table", 3))
    {
        // Get a pointer to the bold font.
        ImGuiIO &io = ImGui::GetIO();
        XASSERT(io.Fonts->Fonts.size() >= 2);
        ImFont *pBoldFont = io.Fonts->Fonts[1];

        ImGui::TableNextColumn();

        // Render the temperatures.
        UpdateTemperatures();
        ImGui::PushFont(pBoldFont);
        ImGui::Text("Temperatures:");
        ImGui::PopFont();
        ImGui::Indent();
        ImGui::Text("CPU: %.1f °C", m_CpuTemperature);
        ImGui::Text("GPU: %.1f °C", m_GpuTemperature);
        ImGui::Text("RAM: %.1f °C", m_RamTemperature);
        ImGui::Text("Board: %.1f °C", m_BoardTemperature);
        ImGui::Unindent();

        ImGui::TableNextColumn();

        // Render the IP addresses
        ImGui::PushFont(pBoldFont);
        ImGui::Text("IP addresses:");
        ImGui::PopFont();
        ImGui::Indent();
        ImGui::Text("Debug: %s", m_DebugIpAddress.c_str());
        ImGui::Text("Title: %s", m_TitleIpAddress.c_str());
        ImGui::Unindent();

        ImGui::EndTable();
    }
}

void SystemInfo::UpdateTemperatures()
{
    // Only update the temperatures once per second.
    uint32_t now = GetTickCount();
    if (now - m_LastTemperatureUpdateTickCount < 1000)
        return;

    m_LastTemperatureUpdateTickCount = now;

    // Get the temperatures from the sensors.
    m_CpuTemperature = XexUtils::SMC::GetCpuTemperature();
    m_GpuTemperature = XexUtils::SMC::GetGpuTemperature();
    m_RamTemperature = XexUtils::SMC::GetRamTemperature();
    m_BoardTemperature = XexUtils::SMC::GetBoardTemperature();
}
