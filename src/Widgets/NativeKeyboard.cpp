#include <XexUtils.h>
#include <cstdint>
#include <memory>
#include <string>
#include <xtl.h>

#include "NativeKeyboard.h"

NativeKeyboard::NativeKeyboard()
    : m_Overlapped(), m_State(State_Idle)
{
}

void NativeKeyboard::Show(const std::string &title, const std::string &description, const std::string &defaultText)
{
    if (m_State == State_Pending)
        return;

    m_Title = XexUtils::Formatter::ToWide(title);
    m_Description = XexUtils::Formatter::ToWide(description);
    m_DefaultText = XexUtils::Formatter::ToWide(defaultText);

    const size_t bufferCharCount = 64;
    m_Overlapped = XOVERLAPPED();
    m_Buffer.reset(new wchar_t[bufferCharCount]());
    m_Result.clear();

    uint32_t result = XShowKeyboardUI(
        0,
        VKBD_DEFAULT,
        m_DefaultText.c_str(),
        m_Title.c_str(),
        m_Description.c_str(),
        m_Buffer.get(),
        bufferCharCount,
        &m_Overlapped
    );

    if (result != ERROR_IO_PENDING)
    {
        m_State = State_Error;
        return;
    }

    m_State = State_Pending;
}

void NativeKeyboard::Update()
{
    if (m_State != State_Pending)
        return;

    if (!XHasOverlappedIoCompleted(&m_Overlapped))
        return;

    uint32_t overlappedResult = XGetOverlappedResult(&m_Overlapped, nullptr, TRUE);

    if (overlappedResult == ERROR_SUCCESS)
    {
        m_Result = XexUtils::Formatter::ToNarrow(m_Buffer.get());
        m_State = State_Success;
    }
    else if (overlappedResult == ERROR_CANCELLED)
        m_State = State_Cancelled;
    else
        m_State = State_Error;
}

NativeKeyboard::State NativeKeyboard::GetState() const
{
    return m_State;
}

const std::string &NativeKeyboard::GetResult() const
{
    return m_Result;
}

void NativeKeyboard::Reset()
{
    if (m_State != State_Pending)
        m_State = State_Idle;
}
