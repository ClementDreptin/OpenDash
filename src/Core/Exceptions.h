#pragma once

#include <stdexcept>
#include <xtl.h>

namespace Exceptions
{

long __stdcall HandleSehException(EXCEPTION_POINTERS *pExceptionInfo);

void HandleCppException(const std::exception &exception);

}

class Exception : public std::runtime_error
{
public:
    explicit Exception(const char *format, ...);
};
