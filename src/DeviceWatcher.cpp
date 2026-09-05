#include <xtl.h>
#include <string>
#include <unordered_map>
#include <XexUtils.h>

#include "DeviceWatcher.h"
#include "Exceptions.h"

DeviceWatcher::DeviceWatcher()
{
    InitializeDevices();

    m_NotificationHandle = XNotifyCreateListener(XNOTIFY_SYSTEM);
    if (!m_NotificationHandle)
        throw Exception("[DeviceWatcher] Couldn't create the system notification listener.");
}

XexUtils::Optional<DeviceInfo> DeviceWatcher::Update()
{
    XASSERT(m_NotificationHandle);

    // Passing an ID is required if we want to be notified of anything, even if the message
    // filter can only return one notification.
    DWORD id = 0;
    if (!XNotifyGetNext(m_NotificationHandle, XN_SYS_STORAGEDEVICESCHANGED, &id, nullptr))
        return XexUtils::NullOpt();

    // Update the list of devices if a notification arrived.
    return UpdateDevices();
}

const std::array<DeviceInfo, 2> &DeviceWatcher::GetDevices() const
{
    return m_Devices;
}

void DeviceWatcher::InitializeDevices()
{
    // Register the hard drive.
    DeviceInfo hdd;
    hdd.Name = "hdd:";
    hdd.Path = "\\Device\\Harddisk0\\Partition1\\";
    hdd.Available = IsPathAccessible(hdd.Path);
    m_Devices[0] = hdd;

    // Register the first USB.
    DeviceInfo usb;
    usb.Name = "usb:";
    usb.Path = "\\Device\\Mass0\\";
    usb.Available = IsPathAccessible(usb.Path);
    m_Devices[1] = usb;

    // Mount the available devices.
    for (size_t i = 0; i < m_Devices.size(); i++)
        if (m_Devices[i].Available)
            MountDevice(m_Devices[i]);
}

XexUtils::Optional<DeviceInfo> DeviceWatcher::UpdateDevices()
{
    for (size_t i = 0; i < m_Devices.size(); i++)
    {
        DeviceInfo &deviceInfo = m_Devices[i];

        // If the current device hasn't changed, skip it.
        bool newDeviceAvailability = IsPathAccessible(deviceInfo.Path);
        if (newDeviceAvailability == deviceInfo.Available)
            continue;

        // Insert the new availibility state.
        deviceInfo.Available = newDeviceAvailability;

        // Mount or unmount the device accordingly.
        if (newDeviceAvailability == true)
            MountDevice(deviceInfo);
        else
            UnmountDevice(deviceInfo);

        return deviceInfo;
    }

    return XexUtils::NullOpt();
}

bool DeviceWatcher::IsPathAccessible(const XexUtils::Fs::Path &path)
{
    // The path is an NT device path so we have to use the kernel API to try to open the
    // file.

    // Create the attributes from the path.
    OBJECT_ATTRIBUTES attributes = {};
    STRING pathString = {};
    RtlInitAnsiString(&pathString, path.c_str());
    InitializeObjectAttributes(&attributes, &pathString, 0, nullptr);

    // Try to open the file.
    HANDLE handle = INVALID_HANDLE_VALUE;
    IO_STATUS_BLOCK block = {};
    NTSTATUS status = NtOpenFile(
        &handle,
        FILE_READ_ATTRIBUTES,
        &attributes,
        &block,
        FILE_SHARE_READ,
        0
    );
    NtClose(handle);

    // The path is accessible if trying to open the file succeeded.
    return NT_SUCCESS(status) && handle != INVALID_HANDLE_VALUE;
}

void DeviceWatcher::MountDevice(const DeviceInfo &deviceInfo)
{
    // Collisions can happen when running the app multiple times because symlinks are
    // persistent across title changes.
    HRESULT hr = XexUtils::Fs::MountPath(deviceInfo.Name, deviceInfo.Path.String());
    if (FAILED(hr) && hr != STATUS_OBJECT_NAME_COLLISION)
        throw Exception(
            "[DeviceWatcher] Couldn't mount the %s device at %s.",
            deviceInfo.Name.c_str(),
            deviceInfo.Path.c_str()
        );
}

void DeviceWatcher::UnmountDevice(const DeviceInfo &deviceInfo)
{
    HRESULT hr = XexUtils::Fs::UnmountPath(deviceInfo.Name);
    if (FAILED(hr))
        throw Exception(
            "[DeviceWatcher] Couldn't unmount the %s device at %s.",
            deviceInfo.Name.c_str(),
            deviceInfo.Path.c_str()
        );
}
