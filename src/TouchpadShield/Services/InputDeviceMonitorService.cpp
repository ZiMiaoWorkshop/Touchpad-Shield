#include "pch.h"
#include "Services/InputDeviceMonitorService.h"

#include "Services/InputDeviceEnumerationService.h"
#include "Services/TouchpadStatusService.h"
#include "Services/TouchpadToggleService.h"

#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Devices.Enumeration.PnP.h>
#include <winrt/Windows.Foundation.Collections.h>

using namespace winrt;
using namespace Windows::Devices::Enumeration::Pnp;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

namespace TouchpadShield::Services
{
    void InputDeviceMonitorService::SetMonitoredDevices(std::vector<MonitoredInputDevice> devices)
    {
        m_monitored = std::move(devices);
    }

    void InputDeviceMonitorService::SetEnabled(bool enabled)
    {
        m_enabled = enabled;
    }

    void InputDeviceMonitorService::SetDeviceChangeCallback(DeviceChangeCallback callback)
    {
        m_deviceChangeCallback = std::move(callback);
    }

    void InputDeviceMonitorService::EnsureWatcherStarted()
    {
        if (m_watcherStarted)
        {
            return;
        }

        auto propertyVector = winrt::single_threaded_vector<hstring>(
            InputDeviceEnumerationService::BuildContainerPropertyNamesList());
        m_watcher = PnpObject::CreateWatcher(PnpObjectType::DeviceContainer, propertyVector);

        auto onAdded = [this](PnpObjectWatcher const&, PnpObject const&)
        {
            if (m_deviceChangeCallback)
            {
                m_deviceChangeCallback();
            }
        };

        auto onUpdated = [this](PnpObjectWatcher const&, PnpObjectUpdate const&)
        {
            if (m_deviceChangeCallback)
            {
                m_deviceChangeCallback();
            }
        };

        auto onRemoved = [this](PnpObjectWatcher const&, PnpObjectUpdate const&)
        {
            if (m_deviceChangeCallback)
            {
                m_deviceChangeCallback();
            }
        };

        m_addedToken = m_watcher.Added(onAdded);
        m_updatedToken = m_watcher.Updated(onUpdated);
        m_removedToken = m_watcher.Removed(onRemoved);

        m_watcher.Start();
        m_watcherStarted = true;
        Logger::Info(L"Input device container notifications registered");
    }

    void InputDeviceMonitorService::StopWatcher()
    {
        if (!m_watcherStarted)
        {
            return;
        }

        try
        {
            m_watcher.Added(m_addedToken);
            m_watcher.Updated(m_updatedToken);
            m_watcher.Removed(m_removedToken);
        }
        catch (...)
        {
        }

        m_addedToken = {};
        m_updatedToken = {};
        m_removedToken = {};

        try
        {
            m_watcher.Stop();
        }
        catch (...)
        {
        }

        m_watcher = nullptr;
        m_watcherStarted = false;
        Logger::Info(L"Input device container notifications unregistered");
    }

    void InputDeviceMonitorService::StartWatching()
    {
        EnsureWatcherStarted();
    }

    void InputDeviceMonitorService::StopWatching()
    {
        StopWatcher();
    }

    void InputDeviceMonitorService::ReconcileNow(std::vector<InputDeviceInfo> const& onlineDevices)
    {
        if (!m_enabled)
        {
            return;
        }

        InputDeviceEnumerationService enumeration{};
        TouchpadStatusService statusService{};
        TouchpadToggleService toggleService{};

        const int onlineCount = enumeration.CountMonitoredOnline(m_monitored, onlineDevices);
        const auto enabled = statusService.IsEnabled();

        Logger::Info(
            L"Input device reconcile: onlineCount=" + std::to_wstring(onlineCount) +
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
}
