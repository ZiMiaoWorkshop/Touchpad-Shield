#include "pch.h"
#include "Services/InputDeviceEnumerationService.h"
#include "Services/InputDeviceTypes.h"
#include "Services/StringUtils.h"

#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Devices.Enumeration.PnP.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Devices.Enumeration.PnP.h>
#include <winrt/Windows.Foundation.Collections.h>

using namespace winrt;
using namespace Windows::Devices::Enumeration::Pnp;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

namespace TouchpadShield::Services
{
    namespace
    {
        bool TryGetBooleanProperty(
            IPropertyValue propertyValue,
            bool defaultValue)
        {
            if (!propertyValue)
            {
                return defaultValue;
            }

            try
            {
                switch (propertyValue.Type())
                {
                case PropertyType::Boolean:
                    return propertyValue.GetBoolean();
                default:
                    return defaultValue;
                }
            }
            catch (...)
            {
                return defaultValue;
            }
        }

        std::wstring TryGetStringProperty(IPropertyValue propertyValue)
        {
            if (!propertyValue)
            {
                return L"";
            }

            try
            {
                if (propertyValue.Type() == PropertyType::String)
                {
                    return propertyValue.GetString().c_str();
                }
            }
            catch (...)
            {
            }

            return L"";
        }

        std::vector<std::wstring> TryGetStringArrayProperty(IPropertyValue propertyValue)
        {
            std::vector<std::wstring> values;
            if (!propertyValue)
            {
                return values;
            }

            try
            {
                if (propertyValue.Type() != PropertyType::StringArray)
                {
                    return values;
                }

                com_array<hstring> items{};
                propertyValue.GetStringArray(items);
                for (hstring const& item : items)
                {
                    values.emplace_back(item.c_str());
                }
            }
            catch (...)
            {
            }

            return values;
        }

        bool IsInputCategory(std::vector<std::wstring> const& categoryIds)
        {
            for (auto const& category : categoryIds)
            {
                if (category.rfind(L"Input.", 0) == 0)
                {
                    return true;
                }
            }
            return false;
        }

        bool IsInternalTouchpadContainer(
            std::wstring const& label,
            std::vector<std::wstring> const& categoryIds)
        {
            for (auto const& category : categoryIds)
            {
                if (ContainsInsensitive(category, L"Input.Touchpad") ||
                    ContainsInsensitive(category, L"Input.TouchPad"))
                {
                    return true;
                }
            }

            return ContainsInsensitive(label, L"Touch Pad") ||
                ContainsInsensitive(label, L"Touchpad") ||
                ContainsInsensitive(label, L"Precision Touch") ||
                ContainsInsensitive(label, L"Trackpad");
        }

        bool TryParseContainerIdFromPnpObjectId(std::wstring const& objectId, GUID& containerId)
        {
            if (TryParseContainerId(objectId, containerId))
            {
                return true;
            }

            const auto braceStart = objectId.find(L'{');
            const auto braceEnd = objectId.find(L'}');
            if (braceStart != std::wstring::npos && braceEnd != std::wstring::npos && braceEnd > braceStart)
            {
                return TryParseContainerId(objectId.substr(braceStart, braceEnd - braceStart + 1), containerId);
            }

            return false;
        }

        std::optional<InputDeviceInfo> ParseInputDevice(PnpObject const& object)
        {
            GUID containerId{};
            if (!TryParseContainerIdFromPnpObjectId(object.Id().c_str(), containerId))
            {
                return std::nullopt;
            }

            auto const properties = object.Properties();
            auto const labelValue = properties.TryLookup(L"System.ItemNameDisplay").try_as<IPropertyValue>();
            auto const connectedValue = properties.TryLookup(L"System.Devices.Connected").try_as<IPropertyValue>();
            auto const pairedValue = properties.TryLookup(L"System.Devices.Paired").try_as<IPropertyValue>();
            auto const localMachineValue = properties.TryLookup(L"System.Devices.LocalMachine").try_as<IPropertyValue>();
            auto const categoryValue = properties.TryLookup(L"System.Devices.CategoryIds").try_as<IPropertyValue>();
            auto const modelIdValue = properties.TryLookup(L"System.Devices.ModelId").try_as<IPropertyValue>();

            if (TryGetBooleanProperty(localMachineValue, false))
            {
                return std::nullopt;
            }

            const std::vector<std::wstring> categoryIds = TryGetStringArrayProperty(categoryValue);
            if (!IsInputCategory(categoryIds))
            {
                return std::nullopt;
            }

            std::wstring label = TryGetStringProperty(labelValue);
            if (label.empty())
            {
                label = L"Input Device";
            }

            if (IsInternalTouchpadContainer(label, categoryIds))
            {
                return std::nullopt;
            }

            const bool connected = TryGetBooleanProperty(connectedValue, false);
            const bool paired = TryGetBooleanProperty(pairedValue, false);
            if (!connected && !paired)
            {
                return std::nullopt;
            }

            InputDeviceInfo device{};
            device.containerId = containerId;
            device.label = std::move(label);
            device.matchKey = BuildDeviceMatchKey(device.label, TryGetStringProperty(modelIdValue));
            device.connected = connected;
            return device;
        }

        void DedupeDevicesByMatchKey(std::vector<InputDeviceInfo>& devices)
        {
            std::vector<InputDeviceInfo> unique;
            unique.reserve(devices.size());

            for (auto& device : devices)
            {
                if (device.matchKey.empty())
                {
                    unique.push_back(std::move(device));
                    continue;
                }

                auto existing = std::find_if(
                    unique.begin(),
                    unique.end(),
                    [&](InputDeviceInfo const& candidate)
                    {
                        return candidate.matchKey == device.matchKey;
                    });

                if (existing == unique.end())
                {
                    unique.push_back(std::move(device));
                    continue;
                }

                if (device.connected && !existing->connected)
                {
                    *existing = std::move(device);
                }
            }

            devices.swap(unique);
        }
    }

    std::vector<hstring> InputDeviceEnumerationService::BuildContainerPropertyNamesList()
    {
        static const std::wstring kPropertyNames[] = {
            L"System.ItemNameDisplay",
            L"System.Devices.Connected",
            L"System.Devices.Paired",
            L"System.Devices.CategoryIds",
            L"System.Devices.LocalMachine",
            L"System.Devices.ModelId",
        };

        std::vector<hstring> properties;
        properties.reserve(std::size(kPropertyNames));
        for (auto const& name : kPropertyNames)
        {
            properties.emplace_back(name);
        }
        return properties;
    }

    std::vector<InputDeviceInfo> InputDeviceEnumerationService::ListInputDevices() const
    {
        std::vector<InputDeviceInfo> devices;
        try
        {
            auto propertyVector = winrt::single_threaded_vector<hstring>(BuildContainerPropertyNamesList());
            PnpObjectCollection const containers = PnpObject::FindAllAsync(
                PnpObjectType::DeviceContainer,
                propertyVector).get();

            for (PnpObject const& object : containers)
            {
                if (auto device = ParseInputDevice(object))
                {
                    devices.push_back(std::move(*device));
                }
            }

            std::sort(
                devices.begin(),
                devices.end(),
                [](InputDeviceInfo const& left, InputDeviceInfo const& right)
                {
                    return left.label < right.label;
                });

            DedupeDevicesByMatchKey(devices);
        }
        catch (hresult_error const& ex)
        {
            Logger::Error(L"ListInputDevices failed: " + std::wstring(ex.message()));
        }
        catch (...)
        {
            Logger::Error(L"ListInputDevices failed with unknown error");
        }

        return devices;
    }

    int InputDeviceEnumerationService::CountMonitoredOnline(
        std::vector<MonitoredInputDevice> const& monitored,
        std::vector<InputDeviceInfo> const& onlineDevices) const
    {
        if (monitored.empty())
        {
            return 0;
        }

        int count = 0;
        for (auto const& item : monitored)
        {
            const bool online = std::any_of(
                onlineDevices.begin(),
                onlineDevices.end(),
                [&](InputDeviceInfo const& device)
                {
                    return InputDevicesMatch(item, device) && device.connected;
                });
            if (online)
            {
                ++count;
            }
        }

        return count;
    }

    bool InputDeviceEnumerationService::IsMonitoredDeviceOnline(
        MonitoredInputDevice const& monitored,
        std::vector<InputDeviceInfo> const& onlineDevices) const
    {
        return std::any_of(
            onlineDevices.begin(),
            onlineDevices.end(),
            [&](InputDeviceInfo const& device)
            {
                return InputDevicesMatch(monitored, device) && device.connected;
            });
    }
}
