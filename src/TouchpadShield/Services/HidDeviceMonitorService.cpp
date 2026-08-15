#include "pch.h"
#include "Services/HidDeviceMonitorService.h"

#include "Services/HidDeviceEnumerationService.h"
#include "Services/TouchpadStatusService.h"
#include "Services/TouchpadToggleService.h"

#include <commctrl.h>
#include <Dbt.h>
#include <hidclass.h>

#pragma comment(lib, "Comctl32.lib")

namespace TouchpadShield::Services
{
    namespace
    {
        constexpr UINT_PTR kHidMonitorSubclassId = 2;
    }

    void HidDeviceMonitorService::SetMonitoredDevices(std::vector<MonitoredHidDevice> devices)
    {
        m_monitored = std::move(devices);
    }

    void HidDeviceMonitorService::SetEnabled(bool enabled)
    {
        m_enabled = enabled;
    }

    void HidDeviceMonitorService::SetDeviceChangeCallback(DeviceChangeCallback callback)
    {
        m_deviceChangeCallback = std::move(callback);
    }

    void HidDeviceMonitorService::AttachSubclass(HWND hwnd)
    {
        if (!hwnd)
        {
            return;
        }

        m_hwnd = hwnd;
        SetWindowSubclass(hwnd, SubclassProc, kHidMonitorSubclassId, reinterpret_cast<DWORD_PTR>(this));
    }

    bool HidDeviceMonitorService::RegisterNotifications(HWND hwnd)
    {
        UnregisterNotifications();
        AttachSubclass(hwnd);

        if (!hwnd)
        {
            return false;
        }

        DEV_BROADCAST_DEVICEINTERFACE_W filter{};
        filter.dbcc_size = sizeof(filter);
        filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        filter.dbcc_classguid = GUID_DEVINTERFACE_HID;

        m_notificationHandle = RegisterDeviceNotificationW(
            hwnd,
            &filter,
            DEVICE_NOTIFY_WINDOW_HANDLE);

        if (!m_notificationHandle)
        {
            Logger::Error(L"RegisterDeviceNotification failed for HID");
            return false;
        }

        Logger::Info(L"HID device notifications registered");
        return true;
    }

    void HidDeviceMonitorService::UnregisterNotifications()
    {
        if (m_notificationHandle)
        {
            UnregisterDeviceNotification(m_notificationHandle);
            m_notificationHandle = nullptr;
            Logger::Info(L"HID device notifications unregistered");
        }

        if (m_hwnd)
        {
            RemoveWindowSubclass(m_hwnd, SubclassProc, kHidMonitorSubclassId);
            m_hwnd = nullptr;
        }
    }

    void HidDeviceMonitorService::ReconcileNow()
    {
        if (!m_enabled)
        {
            return;
        }

        HidDeviceEnumerationService enumeration{};
        TouchpadStatusService statusService{};
        TouchpadToggleService toggleService{};

        const int onlineCount = enumeration.CountMonitoredOnline(m_monitored);
        const auto enabled = statusService.IsEnabled();

        Logger::Info(
            L"HID reconcile: onlineCount=" + std::to_wstring(onlineCount) +
            L" Enabled=" + (enabled.has_value() ? (enabled.value() ? L"1" : L"0") : L"?"));

        if (onlineCount >= 1 && enabled.has_value() && enabled.value())
        {
            toggleService.RequestEnabledAsync(false);
        }
        else if (onlineCount == 0 && enabled.has_value() && !enabled.value())
        {
            toggleService.RequestEnabledAsync(true);
        }
    }

    bool HidDeviceMonitorService::HandleWindowMessage(UINT msg, WPARAM wParam, LPARAM /*lParam*/)
    {
        if (msg != WM_DEVICECHANGE)
        {
            return false;
        }

        if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE)
        {
            if (m_deviceChangeCallback)
            {
                m_deviceChangeCallback();
            }

            ReconcileNow();
            return true;
        }

        return false;
    }

    LRESULT CALLBACK HidDeviceMonitorService::SubclassProc(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam,
        UINT_PTR /*idSubclass*/,
        DWORD_PTR refData)
    {
        auto* self = reinterpret_cast<HidDeviceMonitorService*>(refData);
        if (self && self->HandleWindowMessage(msg, wParam, lParam))
        {
            return TRUE;
        }

        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }
}
