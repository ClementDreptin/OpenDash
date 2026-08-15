#include <xtl.h>
#include <imgui.h>
#include <imgui_impl_xbox360.h>
#include <imgui_impl_dx9.h>
#include <XexUtils.h>

#include "Exceptions.h"
#include "Renderer.h"

D3DDevice *Renderer::s_pDevice = nullptr;

Renderer::Renderer()
{
    CreateDevice();

    InitImGui();
}

void Renderer::StartFrame()
{
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplXbox360_NewFrame();
    ImGui::NewFrame();
}

void Renderer::EndFrame()
{
    // End the ImGui frame.
    ImGui::EndFrame();

    // Finish setting up the device render state.
    s_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    s_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    s_pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

    // Render the clear color background.
    D3DCOLOR clearColor = D3DCOLOR_XRGB(114, 140, 153);
    s_pDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clearColor, 1.0f, 0);

    // Render ImGui.
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

    // Tell the device to render.
    s_pDevice->Present(nullptr, nullptr, nullptr, nullptr);
}

D3DDevice *Renderer::GetDevice()
{
    return s_pDevice;
}

void Renderer::CreateDevice()
{
    // Create the D3D object.
    Direct3D *pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    (void)pD3D;

    // D3DDevice creation options.
    D3DPRESENT_PARAMETERS d3dpp = {};

    // The definition is always 720p on Xbox 360, other definitions are created by the
    // hardware scaler.
    d3dpp.BackBufferWidth = 1280;
    d3dpp.BackBufferHeight = 720;
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;

    // Depth stencil.
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;

    // VSync.
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    // Create the Direct3D device.
    pD3D->CreateDevice(0, D3DDEVTYPE_HAL, nullptr, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &s_pDevice);
}

void Renderer::InitImGui()
{
    XASSERT(s_pDevice != nullptr);

    // Setup the Dear ImGui context.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // Setup the fonts.
    m_pRegularFont = io.Fonts->AddFontFromFileTTF("game:\\assets\\fonts\\Geist\\Geist-Regular.ttf", 20.0f);
    m_pBoldFont = io.Fonts->AddFontFromFileTTF("game:\\assets\\fonts\\Geist\\Geist-Bold.ttf", 20.0f);
    io.FontDefault = m_pRegularFont;

    // Setup the Dear ImGui style.
    ImGui::StyleColorsDark();

    // Initialize the platform backend.
    if (!ImGui_ImplXbox360_Init())
        throw new Exception("[UI]: Error: Failed to initialized the Xbox 360 backend.");

    // Initialize the renderer backend.
    if (!ImGui_ImplDX9_Init(s_pDevice))
        throw new Exception("[UI]: Error: Failed to initialized the DirectX 9 backend.");
}
