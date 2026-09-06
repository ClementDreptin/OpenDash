#pragma once

#include <utility>

template<typename T>
class ScopeGuard
{
public:
    ScopeGuard(T func)
        : m_Func(func) {}

    ScopeGuard(ScopeGuard &&other)
        : m_Func(std::move(other.m_Func)) {}

    ~ScopeGuard()
    {
        m_Func();
    }

private:
    ScopeGuard(const ScopeGuard &);
    ScopeGuard &operator=(const ScopeGuard &);

    T m_Func;
};

template<typename TFunc>
ScopeGuard<TFunc> MakeScopeGuard(TFunc func)
{
    return ScopeGuard<TFunc>(func);
}
