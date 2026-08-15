#pragma once

#include "Services/HidDeviceTypes.h"

#include <Dbt.h>
#include <functional>
#include <vector>

namespace TouchpadShield::Services
{
    class HidDeviceMonitorService
    {
    public:
        using DeviceChangeCallback = std::function<void()>;

        void SetMonitoredDevices(std::vector<MonitoredHidDevice> devices);
        void SetEnabled(bool enabled);
        bool RegisterNotifications(HWND hwnd);
        void UnregisterNotifications();
        void ReconcileNow();
        void SetDeviceChangeCallback(DeviceChangeCallback callback);

        bool HandleWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    private:
        void AttachSubclass(HWND hwnd);
        static LRESULT CALLBACK SubclassProc(
            HWND hwnd,
            UINT msg,
            WPARAM wParam,
            LPARAM lParam,
            UINT_PTR idSubclass,
            DWORD_PTR refData);

        std::vector<MonitoredHidDevice> m_monitored{};
        HDEVNOTIFY m_notificationHandle{ nullptr };
        HWND m_hwnd{ nullptr };
        bool m_enabled{ false };
        DeviceChangeCallback m_deviceChangeCallback{};
    };
}
