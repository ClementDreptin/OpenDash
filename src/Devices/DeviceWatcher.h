#pragma once

#include <XexUtils.h>
#include <array>
#include <string>
#include <xtl.h>

#include "../Core/Event.h"

struct DeviceInfo
{
    std::string Name;
    XexUtils::Fs::Path Path;
    bool Available;
    bool ReadOnly;
};

class DeviceWatcher : public EventEmitter
{
public:
    DeviceWatcher();

    void Update();

    const std::array<DeviceInfo, 3> &GetDevices() const;

private:
    HANDLE m_NotificationHandle;
    std::array<DeviceInfo, 3> m_Devices;

    void InitializeDevices();

    void UpdateDevices();

    bool IsPathAccessible(const XexUtils::Fs::Path &path);

    void MountDevice(const DeviceInfo &deviceInfo);

    void UnmountDevice(const DeviceInfo &deviceInfo);
};

class DeviceChangedEvent : public Event
{
public:
    DeviceChangedEvent(const DeviceInfo &deviceInfo)
        : m_DeviceInfo(deviceInfo) {}

    const DeviceInfo &GetDeviceInfo() const { return m_DeviceInfo; }

    EVENT_CLASS_TYPE(EventType_DeviceChanged)
private:
    DeviceInfo m_DeviceInfo;
};
