#pragma once

#include "Services/InputDeviceTypes.h"

#include <functional>
#include <vector>

#include <winrt/base.h>
#include <winrt/Windows.Devices.Enumeration.PnP.h>

namespace TouchpadShield::Services
{
    class InputDeviceMonitorService
    {
    public:
        using DeviceChangeCallback = std::function<void()>;

        void SetMonitoredDevices(std::vector<MonitoredInputDevice> devices);
        void SetEnabled(bool enabled);
        void SetDeviceChangeCallback(DeviceChangeCallback callback);
        void StartWatching();
        void StopWatching();
        void ReconcileNow(std::vector<InputDeviceInfo> const& onlineDevices);

    private:
        void EnsureWatcherStarted();
        void StopWatcher();

        std::vector<MonitoredInputDevice> m_monitored{};
        winrt::Windows::Devices::Enumeration::Pnp::PnpObjectWatcher m_watcher{ nullptr };
        winrt::event_token m_addedToken{};
        winrt::event_token m_updatedToken{};
        winrt::event_token m_removedToken{};
        bool m_enabled{ false };
        bool m_watcherStarted{ false };
        DeviceChangeCallback m_deviceChangeCallback{};
    };
}
