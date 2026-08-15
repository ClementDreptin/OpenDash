#include <xtl.h>
#include <cstdint>
#include <string>
#include <sstream>
#include <vector>
#include <XexUtils.h>

#include "Exceptions.h"
#include "Renderer.h"

namespace Exceptions
{

static std::string ExceptionInfoToString(EXCEPTION_POINTERS *pExceptionInfo)
{
    XASSERT(pExceptionInfo != nullptr);

    EXCEPTION_RECORD *pExceptionRecord = pExceptionInfo->ExceptionRecord;
    std::stringstream text;

    // Stringify the exception code.
    switch (pExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        text << "EXCEPTION_ACCESS_VIOLATION";
        break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        text << "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
        break;
    case EXCEPTION_BREAKPOINT:
        text << "EXCEPTION_BREAKPOINT";
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        text << "EXCEPTION_DATATYPE_MISALIGNMENT";
        break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        text << "EXCEPTION_FLT_DENORMAL_OPERAND";
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        text << "EXCEPTION_FLT_DIVIDE_BY_ZERO";
        break;
    case EXCEPTION_FLT_INEXACT_RESULT:
        text << "EXCEPTION_FLT_INEXACT_RESULT";
        break;
    case EXCEPTION_FLT_INVALID_OPERATION:
        text << "EXCEPTION_FLT_INVALID_OPERATION";
        break;
    case EXCEPTION_FLT_OVERFLOW:
        text << "EXCEPTION_FLT_OVERFLOW";
        break;
    case EXCEPTION_FLT_STACK_CHECK:
        text << "EXCEPTION_FLT_STACK_CHECK";
        break;
    case EXCEPTION_FLT_UNDERFLOW:
        text << "EXCEPTION_FLT_UNDERFLOW";
        break;
    case EXCEPTION_GUARD_PAGE:
        text << "EXCEPTION_GUARD_PAGE";
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        text << "EXCEPTION_ILLEGAL_INSTRUCTION";
        break;
    case EXCEPTION_IN_PAGE_ERROR:
        text << "EXCEPTION_IN_PAGE_ERROR";
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        text << "EXCEPTION_INT_DIVIDE_BY_ZERO";
        break;
    case EXCEPTION_INT_OVERFLOW:
        text << "EXCEPTION_INT_OVERFLOW";
        break;
    case EXCEPTION_INVALID_DISPOSITION:
        text << "EXCEPTION_INVALID_DISPOSITION";
        break;
    case EXCEPTION_INVALID_HANDLE:
        text << "EXCEPTION_INVALID_HANDLE";
        break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        text << "EXCEPTION_NONCONTINUABLE_EXCEPTION";
        break;
    case EXCEPTION_PRIV_INSTRUCTION:
        text << "EXCEPTION_PRIV_INSTRUCTION";
        break;
    case EXCEPTION_SINGLE_STEP:
        text << "EXCEPTION_SINGLE_STEP";
        break;
    case EXCEPTION_STACK_OVERFLOW:
        text << "EXCEPTION_STACK_OVERFLOW";
        break;
    default:
        text << "EXCEPTION_UNKNOWN";
    }

    // Add where the exception occurred.
    text << " at address " << std::hex << pExceptionRecord->ExceptionAddress << '.';

    // Segfaults are the only exceptions with a documented array structure for
    // ExceptionInformation. The details of the contents can be found here:
    // https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-exception_record
    if (pExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    {
        // According to the docs, this should always be 2. We have a problem if it's not.
        if (pExceptionRecord->NumberParameters != 2)
            return text.str();

        uint32_t flag = pExceptionRecord->ExceptionInformation[0];
        uint32_t address = pExceptionRecord->ExceptionInformation[1];

        // The flag should either be 0 to indicate a read, or 1 to indicate a write.
        // If it's anything else, we have a problem.
        if (flag > 1)
            return text.str();

        // Add the details of the segfault.
        text << "\nFailed to ";
        text << (flag == 0 ? "read" : "write");
        text << " at address " << std::hex << address << '.';
    }

    return text.str();
}

static void DisplayErrorMessageBox(const std::string &errorMessage)
{
    // We suspend the device so that Xam can take ownership of it and take care of
    // rendering the message box.
    D3DDevice *pDevice = Renderer::GetDevice();
    if (pDevice != nullptr)
        pDevice->Suspend();

    // Convert the error message to a wide string because that's the encoding the message
    // box expects.
    std::wstring wideErrorMessage = XexUtils::Formatter::ToWide(errorMessage);

    // Render the message box.
    std::vector<std::wstring> buttonLabels(2);
    buttonLabels[0] = L"Go back to dashboard";
    buttonLabels[1] = L"Reboot";
    uint32_t pressedButtonIndex = 0;
    uint32_t result = XexUtils::Xam::ShowMessageBox(
        L"Uncaught exception",
        XexUtils::Formatter::Format(
            L"OpenDash crashed with the following error:\n%s\n\nWhat would you like to do?",
            wideErrorMessage.c_str()
        ),
        buttonLabels,
        XMB_ERRORICON,
        &pressedButtonIndex
    );

    // Reboot if the user pressed the "Reboot" button.
    if (result == ERROR_SUCCESS && pressedButtonIndex == 1)
        XexUtils::Reboot();

    // Go back to the official dashboard in any other case (pressed "Go back to dashboard"
    // or cancelled the message box).
    XLaunchNewImage("", 0);
}

void HandleCppException(const std::exception &exception)
{
    DisplayErrorMessageBox(exception.what());
}

long __stdcall HandleSehException(EXCEPTION_POINTERS *pExceptionInfo)
{
    DisplayErrorMessageBox(ExceptionInfoToString(pExceptionInfo));

    return EXCEPTION_CONTINUE_EXECUTION;
}

}

Exception::Exception(const char *format, ...)
    : std::runtime_error(XexUtils::Formatter::Format(format))
{
}
