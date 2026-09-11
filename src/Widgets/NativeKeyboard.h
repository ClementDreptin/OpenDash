#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <xtl.h>

class NativeKeyboard
{
public:
    enum State
    {
        State_Idle,
        State_Pending,
        State_Success,
        State_Cancelled,
        State_Error,
    };

    NativeKeyboard();

    void Show(const std::string &title, const std::string &description, const std::string &defaultText = "");

    void Update();

    State GetState() const;

    const std::string &GetResult() const;

    void Reset();

private:
    std::wstring m_Title;
    std::wstring m_Description;
    std::wstring m_DefaultText;
    std::unique_ptr<wchar_t[]> m_Buffer;
    std::string m_Result;
    XOVERLAPPED m_Overlapped;
    State m_State;
};
