#pragma once

#include "Services/HidDeviceTypes.h"

#include <vector>

namespace TouchpadShield::Services
{
    class HidDeviceEnumerationService
    {
    public:
        std::vector<HidDeviceInfo> ListConnectedHidDevices(bool excludeInternalTouchpad = true) const;
        int CountMonitoredOnline(std::vector<MonitoredHidDevice> const& monitored) const;

    private:
        static bool IsInternalTouchpad(std::wstring const& label, std::wstring const& hardwareId);
        static bool ParseVidPid(std::wstring const& devicePath, std::wstring& vid, std::wstring& pid);
    };
}
