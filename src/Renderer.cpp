#include <xtl.h>
#include <cstdint>
#include <imgui.h>
#include <imgui_impl_xbox360.h>
#include <imgui_impl_dx9.h>
#include <XexUtils.h>

#include "Exceptions.h"
#include "Renderer.h"

D3DDevice *Renderer::s_pDevice = nullptr;

// The definition is always 720p on Xbox 360, other definitions are created by the
// hardware scaler.
const float Renderer::s_DisplayWidth = 1280.0f;
const float Renderer::s_DisplayHeight = 720.0f;
const float Renderer::s_SafeAreaMultipler = 0.05f;

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
    D3DCOLOR clearColor = D3DCOLOR_XRGB(9, 9, 11);
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

Renderer::Area Renderer::GetSafeArea()
{
    Area area = {};
    area.Origin = XexUtils::Math::vec2(s_DisplayWidth * s_SafeAreaMultipler, s_DisplayHeight * s_SafeAreaMultipler);
    area.Width = s_DisplayWidth * (1.0f - s_SafeAreaMultipler * 2.0f);
    area.Height = s_DisplayHeight * (1.0f - s_SafeAreaMultipler * 2.0f);

    return area;
}

void Renderer::CreateDevice()
{
    // Create the D3D object.
    Direct3D *pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    (void)pD3D;

    // D3DDevice creation options.
    D3DPRESENT_PARAMETERS d3dpp = {};
    d3dpp.BackBufferWidth = static_cast<uint32_t>(s_DisplayWidth);
    d3dpp.BackBufferHeight = static_cast<uint32_t>(s_DisplayHeight);
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

    // Register the regular font.
    m_pRegularFont = io.Fonts->AddFontFromFileTTF("game:\\assets\\fonts\\Geist-Regular.ttf", 24.0f);

    // Load the symbol font and merge it into the regular font.
    ImFontConfig symbolFontConfig;
    symbolFontConfig.MergeMode = true;
    ImWchar symbolFontRanges[] = { 0xF020, 0xF0FD, 0 };
    io.Fonts->AddFontFromFileTTF("game:\\assets\\fonts\\Convsym.ttf", 24.0f, &symbolFontConfig, symbolFontRanges);

    // Register the bold font.
    m_pBoldFont = io.Fonts->AddFontFromFileTTF("game:\\assets\\fonts\\Geist-Bold.ttf", 24.0f);

    // Make the regular font the default font.
    io.FontDefault = m_pRegularFont;

    // Setup the Dear ImGui style.
    ImGui::StyleColorsDark();

    // Initialize the platform backend.
    if (!ImGui_ImplXbox360_Init())
        throw Exception("[Renderer]: Failed to initialized the Xbox 360 backend.");

    // Initialize the renderer backend.
    if (!ImGui_ImplDX9_Init(s_pDevice))
        throw Exception("[Renderer]: Failed to initialized the DirectX 9 backend.");
}

Texture::Texture(const XexUtils::Fs::Path &filePath)
    : m_pTexture(nullptr), m_Width(0.0f), m_Height(0.0f)
{
    XASSERT(!filePath.IsEmpty());

    // Create the texture from the file path.
    HRESULT hr = D3DXCreateTextureFromFile(Renderer::GetDevice(), filePath.c_str(), &m_pTexture);
    if (FAILED(hr))
        throw Exception("[Renderer]: Couldn't create a texture from %s (%X).", filePath.c_str(), hr);

    // Get the width and the height from the texture description.
    D3DSURFACE_DESC description = {};
    m_pTexture->GetLevelDesc(0, &description);
    m_Width = static_cast<float>(description.Width);
    m_Height = static_cast<float>(description.Height);
}

Texture::Texture(const XexUtils::Fs::Path &filePath, float width, float height)
    : m_pTexture(nullptr), m_Width(width), m_Height(height)
{
    XASSERT(!filePath.IsEmpty());

    // Create the texture from the file path and the dimensions.
    HRESULT hr = D3DXCreateTextureFromFileEx(
        Renderer::GetDevice(),
        filePath.c_str(),
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        D3DX_DEFAULT,
        0,
        D3DFMT_UNKNOWN,
        D3DPOOL_MANAGED,
        D3DX_DEFAULT,
        D3DX_DEFAULT,
        0,
        nullptr,
        nullptr,
        &m_pTexture
    );
    if (FAILED(hr))
        throw Exception("[Renderer]: Couldn't create a texture from %s (%X).", filePath.c_str(), hr);
}

Texture::~Texture()
{
    Release();
}

Texture::Texture(const Texture &other)
    : m_pTexture(other.m_pTexture)
{
    AddRef();
}

Texture &Texture::operator=(const Texture &other)
{
    if (this == &other)
        return *this;

    Release();
    m_pTexture = other.m_pTexture;
    AddRef();

    return *this;
}

D3DTexture *Texture::GetHandle() const
{
    return m_pTexture;
}

float Texture::GetWidth() const
{
    return m_Width;
}

float Texture::GetHeight() const
{
    return m_Height;
}

void Texture::AddRef()
{
    if (m_pTexture != nullptr)
        m_pTexture->AddRef();
}

void Texture::Release()
{
    if (m_pTexture != nullptr)
    {
        m_pTexture->Release();
        m_pTexture = nullptr;
    }
}
