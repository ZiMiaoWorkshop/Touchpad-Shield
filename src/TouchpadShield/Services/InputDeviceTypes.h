#pragma once

#include <guiddef.h>

#include <string>
#include <vector>

namespace TouchpadShield::Services
{
    struct InputDeviceInfo
    {
        GUID containerId{};
        std::wstring label;
        std::wstring matchKey;
        bool connected{ false };
    };

    struct MonitoredInputDevice
    {
        GUID containerId{};
        std::wstring label;
        std::wstring matchKey;
    };

    std::wstring ContainerIdToString(GUID const& containerId);
    bool TryParseContainerId(std::wstring const& text, GUID& containerId);
    std::wstring BuildDeviceMatchKey(std::wstring const& label, std::wstring const& modelId);
    bool InputDevicesMatch(MonitoredInputDevice const& monitored, InputDeviceInfo const& online);
}
