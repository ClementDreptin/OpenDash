#pragma once

#include <xtl.h>

class Renderer
{
public:
    Renderer();

    void StartFrame();

    void EndFrame();

private:
    Direct3D *m_pD3D;
    D3DDevice *m_pDevice;

private:
    void CreateDevice();

    void InitImGui();
};
