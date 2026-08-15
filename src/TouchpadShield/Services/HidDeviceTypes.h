#pragma once

#include <string>
#include <vector>

namespace TouchpadShield::Services
{
    struct HidDeviceInfo
    {
        std::wstring vid;
        std::wstring pid;
        std::wstring label;
    };

    struct MonitoredHidDevice
    {
        std::wstring vid;
        std::wstring pid;
        std::wstring label;
    };

    bool HidDevicesMatch(MonitoredHidDevice const& monitored, HidDeviceInfo const& online);
}
