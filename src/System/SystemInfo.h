#pragma once

#include <cstdint>

#include "../Core/Scene.h"

class SystemInfo : public Scene
{
public:
    SystemInfo();

    void Render() override;

private:
    float m_CpuTemperature;
    float m_GpuTemperature;
    float m_RamTemperature;
    float m_BoardTemperature;
    uint32_t m_LastTemperatureUpdateTickCount;
    std::string m_DebugIpAddress;
    std::string m_TitleIpAddress;

    void UpdateTemperatures();
};
