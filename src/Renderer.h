#pragma once

#include <xtl.h>
#include <imgui.h>

class Renderer
{
public:
    Renderer();

    void StartFrame();

    void EndFrame();

    static D3DDevice *GetDevice();

private:
    ImFont *m_pRegularFont;
    ImFont *m_pBoldFont;

    static D3DDevice *s_pDevice;

private:
    void CreateDevice();

    void InitImGui();
};
