#include <cstdint>
#include <imgui.h>

#include "Theme.h"

namespace Theme
{

namespace Colors
{

ImVec4 Rosewater;
ImVec4 Flamingo;
ImVec4 Pink;
ImVec4 Mauve;
ImVec4 Red;
ImVec4 Maroon;
ImVec4 Peach;
ImVec4 Yellow;
ImVec4 Green;
ImVec4 Teal;
ImVec4 Sky;
ImVec4 Sapphire;
ImVec4 Blue;
ImVec4 Lavender;
ImVec4 Text;
ImVec4 Subtext1;
ImVec4 Subtext0;
ImVec4 Overlay2;
ImVec4 Overlay1;
ImVec4 Overlay0;
ImVec4 Surface2;
ImVec4 Surface1;
ImVec4 Surface0;
ImVec4 Base;
ImVec4 Mantle;
ImVec4 Crust;

}

static ImVec4 HexToImVec4(uint32_t hexColor, float alpha = 1.0f)
{
    float r = static_cast<float>((hexColor >> 16) & 0xFF) / 255.0f;
    float g = static_cast<float>((hexColor >> 8) & 0xFF) / 255.0f;
    float b = static_cast<float>(hexColor & 0xFF) / 255.0f;

    return ImVec4(r, g, b, alpha);
}

static void ApplyColorScheme()
{
    using namespace Colors;

    ImGuiStyle &style = ImGui::GetStyle();
    ImVec4 *colors = style.Colors;

    // Set the Catppuccin palette to Mocha.
    Rosewater = HexToImVec4(0xF5E0DC);
    Flamingo = HexToImVec4(0xF2CDCD);
    Pink = HexToImVec4(0xF5C2E7);
    Mauve = HexToImVec4(0xCBA6F7);
    Red = HexToImVec4(0xF38BA8);
    Maroon = HexToImVec4(0xEBA0AC);
    Peach = HexToImVec4(0xFAB387);
    Yellow = HexToImVec4(0xF9E2AF);
    Green = HexToImVec4(0xA6E3A1);
    Teal = HexToImVec4(0x94E2D5);
    Sky = HexToImVec4(0x89DCEB);
    Sapphire = HexToImVec4(0x74C7EC);
    Blue = HexToImVec4(0x89B4FA);
    Lavender = HexToImVec4(0xB4BEFE);
    Text = HexToImVec4(0xCDD6F4);
    Subtext1 = HexToImVec4(0xBAC2DE);
    Subtext0 = HexToImVec4(0xA6ADC8);
    Overlay2 = HexToImVec4(0x9399B2);
    Overlay1 = HexToImVec4(0x7F849C);
    Overlay0 = HexToImVec4(0x6C7086);
    Surface2 = HexToImVec4(0x585B70);
    Surface1 = HexToImVec4(0x45475A);
    Surface0 = HexToImVec4(0x313244);
    Base = HexToImVec4(0x1E1E2E);
    Mantle = HexToImVec4(0x181825);
    Crust = HexToImVec4(0x11111B);

    // Apply the palette.
    colors[ImGuiCol_WindowBg] = Base;
    colors[ImGuiCol_ChildBg] = Base;
    colors[ImGuiCol_PopupBg] = Base;
    colors[ImGuiCol_Border] = Surface1;
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_FrameBg] = Surface0;
    colors[ImGuiCol_FrameBgHovered] = Surface1;
    colors[ImGuiCol_FrameBgActive] = Surface2;
    colors[ImGuiCol_TitleBg] = Mantle;
    colors[ImGuiCol_TitleBgActive] = Surface0;
    colors[ImGuiCol_TitleBgCollapsed] = Mantle;
    colors[ImGuiCol_MenuBarBg] = Mantle;
    colors[ImGuiCol_ScrollbarBg] = Surface0;
    colors[ImGuiCol_ScrollbarGrab] = Surface2;
    colors[ImGuiCol_ScrollbarGrabHovered] = Overlay0;
    colors[ImGuiCol_ScrollbarGrabActive] = Overlay2;
    colors[ImGuiCol_CheckMark] = Green;
    colors[ImGuiCol_SliderGrab] = Sapphire;
    colors[ImGuiCol_SliderGrabActive] = Blue;
    colors[ImGuiCol_Button] = Surface0;
    colors[ImGuiCol_ButtonHovered] = Surface1;
    colors[ImGuiCol_ButtonActive] = Surface2;
    colors[ImGuiCol_Header] = Surface0;
    colors[ImGuiCol_HeaderHovered] = Surface1;
    colors[ImGuiCol_HeaderActive] = Surface2;
    colors[ImGuiCol_Separator] = Surface1;
    colors[ImGuiCol_SeparatorHovered] = Mauve;
    colors[ImGuiCol_SeparatorActive] = Mauve;
    colors[ImGuiCol_ResizeGrip] = Surface2;
    colors[ImGuiCol_ResizeGripHovered] = Mauve;
    colors[ImGuiCol_ResizeGripActive] = Mauve;
    colors[ImGuiCol_Tab] = Surface0;
    colors[ImGuiCol_TabHovered] = Surface2;
    colors[ImGuiCol_TabActive] = Surface1;
    colors[ImGuiCol_TabUnfocused] = Surface0;
    colors[ImGuiCol_TabUnfocusedActive] = Surface1;
    colors[ImGuiCol_PlotLines] = Blue;
    colors[ImGuiCol_PlotLinesHovered] = Peach;
    colors[ImGuiCol_PlotHistogram] = Teal;
    colors[ImGuiCol_PlotHistogramHovered] = Green;
    colors[ImGuiCol_TableHeaderBg] = Surface0;
    colors[ImGuiCol_TableBorderStrong] = Surface1;
    colors[ImGuiCol_TableBorderLight] = Surface0;
    colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = Surface2;
    colors[ImGuiCol_DragDropTarget] = Yellow;
    colors[ImGuiCol_NavHighlight] = Lavender;
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.35f);
    colors[ImGuiCol_Text] = Text;
    colors[ImGuiCol_TextDisabled] = Subtext0;
}

static void ApplyStyles()
{
    ImGuiStyle &style = ImGui::GetStyle();

    // Rounded corners.
    style.WindowRounding = 6.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;

    // Padding and spacing.
    style.WindowPadding = ImVec2(14.0f, 14.0f);
    style.FramePadding = ImVec2(14.0f, 10.0f);
    style.ItemSpacing = ImVec2(12.0f, 12.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.IndentSpacing = 21.0f;
    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 10.0f;

    // Borders.
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;
}

void Apply()
{
    ApplyColorScheme();

    ApplyStyles();
}

}
