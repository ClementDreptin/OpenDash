#pragma once

#include <xtl.h>
#include <imgui.h>
#include <XexUtils.h>

class Renderer
{
public:
    Renderer();

    void StartFrame();

    void EndFrame();

    static D3DDevice *GetDevice();

    struct Area
    {
        XexUtils::Math::vec2 Origin;
        float Width;
        float Height;
    };

    static Area GetSafeArea();

private:
    ImFont *m_pRegularFont;
    ImFont *m_pBoldFont;

    static D3DDevice *s_pDevice;
    static const float s_DisplayWidth;
    static const float s_DisplayHeight;
    static const float s_SafeAreaMultipler;

private:
    void CreateDevice();

    void InitImGui();
};

class Texture
{
public:
    Texture(const XexUtils::Fs::Path &filePath);

    Texture(const XexUtils::Fs::Path &filePath, float width, float height);

    ~Texture();

    Texture(const Texture &other);

    Texture &operator=(const Texture &other);

    D3DTexture *GetHandle() const;

    float GetWidth() const;

    float GetHeight() const;

private:
    D3DTexture *m_pTexture;
    float m_Width;
    float m_Height;

    void AddRef();

    void Release();
};
