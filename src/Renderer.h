#pragma once

#include <xtl.h>

#include <imgui.h>

class Renderer
{
public:
    Renderer();

    void StartFrame();

    void EndFrame();

private:
    Direct3D *m_pD3D;
    D3DDevice *m_pDevice;
    ImFont *m_pRegularFont;
    ImFont *m_pBoldFont;

private:
    void CreateDevice();

    void InitImGui();
};
