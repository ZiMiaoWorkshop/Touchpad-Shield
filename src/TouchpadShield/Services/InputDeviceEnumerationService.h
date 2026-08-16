#pragma once

#include "Services/InputDeviceTypes.h"

#include <vector>

#include <winrt/base.h>

namespace TouchpadShield::Services
{
    class InputDeviceEnumerationService
    {
    public:
        std::vector<InputDeviceInfo> ListInputDevices() const;

        int CountMonitoredOnline(
            std::vector<MonitoredInputDevice> const& monitored,
            std::vector<InputDeviceInfo> const& onlineDevices) const;

        bool IsMonitoredDeviceOnline(
            MonitoredInputDevice const& monitored,
            std::vector<InputDeviceInfo> const& onlineDevices) const;

        static std::vector<winrt::hstring> BuildContainerPropertyNamesList();
    };
}
