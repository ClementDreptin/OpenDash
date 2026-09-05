#pragma once

#include <xtl.h>
#include <array>
#include <string>
#include <XexUtils.h>

struct DeviceInfo
{
    std::string Name;
    XexUtils::Fs::Path Path;
    bool Available;
};

class DeviceWatcher
{
public:
    DeviceWatcher();

    XexUtils::Optional<DeviceInfo> Update();

    const std::array<DeviceInfo, 2> &GetDevices() const;

private:
    HANDLE m_NotificationHandle;
    std::array<DeviceInfo, 2> m_Devices;

    void InitializeDevices();

    XexUtils::Optional<DeviceInfo> UpdateDevices();

    bool IsPathAccessible(const XexUtils::Fs::Path &path);

    void MountDevice(const DeviceInfo &deviceInfo);

    void UnmountDevice(const DeviceInfo &deviceInfo);
};
