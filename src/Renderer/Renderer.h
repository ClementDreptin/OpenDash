#pragma once

#include <XexUtils.h>
#include <imgui.h>
#include <xtl.h>

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

// Macros for the UTF-8 bytes of the icons present in the Convsym font.
// These macros are automatically generated with the generate-char-macros.ps1 script,
// don't edit these manually!
#define CHAR_BUTTON_A "\xEF\x81\x81"     // U+F041
#define CHAR_BUTTON_B "\xEF\x81\x82"     // U+F042
#define CHAR_BUTTON_X "\xEF\x81\x98"     // U+F058
#define CHAR_BUTTON_Y "\xEF\x81\x99"     // U+F059
#define CHAR_BUTTON_LB "\xEF\x81\x9F"    // U+F05F
#define CHAR_BUTTON_RB "\xEF\x81\xA0"    // U+F060
#define CHAR_BUTTON_BACK "\xEF\x80\xBA"  // U+F03A
#define CHAR_BUTTON_START "\xEF\x80\xBB" // U+F03B
