#include "pch.h"
#include "Services/HidDeviceEnumerationService.h"

#include <cfgmgr32.h>
#include <devpropdef.h>
#include <devquery.h>
#include <hidclass.h>
#include <optional>
#include <setupapi.h>

#pragma comment(lib, "Setupapi.lib")
#pragma comment(lib, "Cfgmgr32.lib")
#pragma comment(lib, "OneCore.lib")

namespace TouchpadShield::Services
{
    namespace
    {
        DEVPROPKEY const kDevPropBusReportedDeviceDesc = {
            {0x540b947e, 0x8b40, 0x45bc, 0xa8, 0xa2, 0x6a, 0x0b, 0x89, 0x4c, 0xbd, 0x02},
            4};
        DEVPROPKEY const kDevPropDeviceFriendlyName = {
            {0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0},
            14};
        DEVPROPKEY const kDevPropDeviceInterfaceFriendlyName = {
            {0x026e516e, 0xb814, 0x414b, 0x83, 0xcd, 0x4d, 0xf7, 0xb4, 0x2c, 0x5f, 0xbc},
            2};
        DEVPROPKEY const kDevPropContainerId = {
            {0x8c7ed206, 0x3f8a, 0x4827, 0xb3, 0xab, 0xae, 0x9e, 0x1f, 0xae, 0xfc, 0x6c},
            2};
        DEVPROPKEY const kDevPropContainerFriendlyName = {
            {0x656a3bb3, 0xecc0, 0x43fd, 0x84, 0x77, 0x4a, 0xe0, 0x40, 0x4a, 0x96, 0xcd},
            12288};
        DEVPROPKEY const kDevPropContainerModelName = {
            {0x656a3bb3, 0xecc0, 0x43fd, 0x84, 0x77, 0x4a, 0xe0, 0x40, 0x4a, 0x96, 0xcd},
            8194};

        bool ContainsInsensitive(std::wstring const& haystack, std::wstring const& needle)
        {
            if (needle.empty())
            {
                return false;
            }

            std::wstring lowerHay = haystack;
            std::wstring lowerNeedle = needle;
            std::transform(lowerHay.begin(), lowerHay.end(), lowerHay.begin(), ::towlower);
            std::transform(lowerNeedle.begin(), lowerNeedle.end(), lowerNeedle.begin(), ::towlower);
            return lowerHay.find(lowerNeedle) != std::wstring::npos;
        }

        std::wstring ToUpperHex(std::wstring value)
        {
            std::transform(value.begin(), value.end(), value.begin(), ::towupper);
            if (value.rfind(L"0X", 0) == 0)
            {
                value.erase(0, 2);
            }
            return value;
        }

        std::wstring ReadDevicePropertyString(
            HDEVINFO deviceInfoSet,
            SP_DEVINFO_DATA const& deviceInfo,
            DEVPROPKEY const& key)
        {
            DEVPROPTYPE propType = DEVPROP_TYPE_EMPTY;
            DWORD requiredSize = 0;
            SetupDiGetDevicePropertyW(
                deviceInfoSet,
                const_cast<SP_DEVINFO_DATA*>(&deviceInfo),
                &key,
                &propType,
                nullptr,
                0,
                &requiredSize,
                0);

            if (requiredSize == 0)
            {
                return L"";
            }

            std::vector<BYTE> buffer(requiredSize);
            if (!SetupDiGetDevicePropertyW(
                    deviceInfoSet,
                    const_cast<SP_DEVINFO_DATA*>(&deviceInfo),
                    &key,
                    &propType,
                    buffer.data(),
                    requiredSize,
                    &requiredSize,
                    0) ||
                propType != DEVPROP_TYPE_STRING)
            {
                return L"";
            }

            return std::wstring(reinterpret_cast<wchar_t*>(buffer.data()));
        }

        std::wstring ReadDeviceInterfacePropertyString(
            HDEVINFO deviceInfoSet,
            SP_DEVICE_INTERFACE_DATA const& interfaceData,
            DEVPROPKEY const& key)
        {
            DEVPROPTYPE propType = DEVPROP_TYPE_EMPTY;
            DWORD requiredSize = 0;
            SetupDiGetDeviceInterfacePropertyW(
                deviceInfoSet,
                const_cast<SP_DEVICE_INTERFACE_DATA*>(&interfaceData),
                &key,
                &propType,
                nullptr,
                0,
                &requiredSize,
                0);

            if (requiredSize == 0)
            {
                return L"";
            }

            std::vector<BYTE> buffer(requiredSize);
            if (!SetupDiGetDeviceInterfacePropertyW(
                    deviceInfoSet,
                    const_cast<SP_DEVICE_INTERFACE_DATA*>(&interfaceData),
                    &key,
                    &propType,
                    buffer.data(),
                    requiredSize,
                    &requiredSize,
                    0) ||
                propType != DEVPROP_TYPE_STRING)
            {
                return L"";
            }

            return std::wstring(reinterpret_cast<wchar_t*>(buffer.data()));
        }

        std::wstring ReadDeviceString(
            HDEVINFO deviceInfoSet,
            SP_DEVINFO_DATA const& deviceInfo,
            DWORD property)
        {
            DWORD requiredSize = 0;
            SetupDiGetDeviceRegistryPropertyW(
                deviceInfoSet,
                const_cast<SP_DEVINFO_DATA*>(&deviceInfo),
                property,
                nullptr,
                nullptr,
                0,
                &requiredSize);

            if (requiredSize == 0)
            {
                return L"";
            }

            std::vector<wchar_t> buffer(requiredSize / sizeof(wchar_t) + 1, L'\0');
            if (!SetupDiGetDeviceRegistryPropertyW(
                    deviceInfoSet,
                    const_cast<SP_DEVINFO_DATA*>(&deviceInfo),
                    property,
                    nullptr,
                    reinterpret_cast<PBYTE>(buffer.data()),
                    requiredSize,
                    nullptr))
            {
                return L"";
            }

            return std::wstring(buffer.data());
        }

        std::wstring ReadDevNodePropertyString(DEVINST devInst, DEVPROPKEY const& key)
        {
            DEVPROPTYPE propType = DEVPROP_TYPE_EMPTY;
            ULONG bufferSize = 0;
            CONFIGRET cr = CM_Get_DevNode_PropertyW(devInst, &key, &propType, nullptr, &bufferSize, 0);
            if (cr != CR_BUFFER_SMALL || bufferSize == 0)
            {
                return L"";
            }

            std::vector<BYTE> buffer(bufferSize);
            cr = CM_Get_DevNode_PropertyW(devInst, &key, &propType, buffer.data(), &bufferSize, 0);
            if (cr != CR_SUCCESS || propType != DEVPROP_TYPE_STRING)
            {
                return L"";
            }

            return std::wstring(reinterpret_cast<wchar_t*>(buffer.data()));
        }

        std::wstring ReadDevNodeRegistryProperty(DEVINST devInst, ULONG property)
        {
            ULONG dataType = 0;
            ULONG bufferSize = 0;
            CONFIGRET cr = CM_Get_DevNode_Registry_PropertyW(
                devInst,
                property,
                &dataType,
                nullptr,
                &bufferSize,
                0);
            if (cr != CR_BUFFER_SMALL || bufferSize == 0)
            {
                return L"";
            }

            std::vector<BYTE> buffer(bufferSize);
            cr = CM_Get_DevNode_Registry_PropertyW(
                devInst,
                property,
                &dataType,
                buffer.data(),
                &bufferSize,
                0);
            if (cr != CR_SUCCESS || (dataType != REG_SZ && dataType != REG_EXPAND_SZ))
            {
                return L"";
            }

            return std::wstring(reinterpret_cast<wchar_t*>(buffer.data()));
        }

        bool IsInfrastructureLabel(std::wstring const& label)
        {
            static std::wstring const patterns[] = {
                L"Root Hub",
                L"根集线器",
                L"Host Controller",
                L"主机控制器",
                L"PCI Express",
                L"ACPI-Compliant System",
                L"ACPI 兼容",
                L"x64 的电脑",
                L"Based PC",
            };

            for (auto const& pattern : patterns)
            {
                if (ContainsInsensitive(label, pattern))
                {
                    return true;
                }
            }

            return false;
        }

        bool IsGenericHidLabel(std::wstring const& label)
        {
            if (label.empty())
            {
                return true;
            }

            static std::wstring const genericPatterns[] = {
                L"HID-compliant mouse",
                L"HID-compliant vendor-defined device",
                L"HID-compliant consumer control device",
                L"HID-compliant system controller",
                L"HID-compliant game controller",
                L"HID Keyboard Device",
                L"HID-compliant device",
                L"符合 HID 标准的鼠标",
                L"符合 HID 标准的游戏控制器",
                L"符合 HID 标准的",
                L"HID 传感器",
                L"HID Sensor",
            };

            for (auto const& pattern : genericPatterns)
            {
                if (ContainsInsensitive(label, pattern))
                {
                    return true;
                }
            }

            return false;
        }

        std::wstring GuidToBracedString(GUID const& guid)
        {
            wchar_t buffer[64] = {};
            swprintf_s(
                buffer,
                L"{%08lX-%04hX-%04hX-%02hX%02hX-%02hX%02hX%02hX%02hX%02hX%02hX}",
                guid.Data1,
                guid.Data2,
                guid.Data3,
                guid.Data4[0],
                guid.Data4[1],
                guid.Data4[2],
                guid.Data4[3],
                guid.Data4[4],
                guid.Data4[5],
                guid.Data4[6],
                guid.Data4[7]);
            return buffer;
        }

        std::optional<GUID> ReadDevNodeGuidProperty(DEVINST devInst, DEVPROPKEY const& key)
        {
            DEVPROPTYPE propType = DEVPROP_TYPE_EMPTY;
            GUID value{};
            ULONG bufferSize = sizeof(value);
            const CONFIGRET result = CM_Get_DevNode_PropertyW(
                devInst,
                &key,
                &propType,
                reinterpret_cast<PBYTE>(&value),
                &bufferSize,
                0);
            if (result != CR_SUCCESS || propType != DEVPROP_TYPE_GUID)
            {
                return std::nullopt;
            }

            return value;
        }

        struct ContainerLabels
        {
            std::wstring friendlyName;
            std::wstring modelName;
        };

        std::wstring ReadContainerPropertyString(
            ULONG propertyCount,
            DEVPROPERTY const* properties,
            DEVPROPKEY const& key)
        {
            const DEVPROPERTY* found =
                DevFindProperty(&key, DEVPROP_STORE_SYSTEM, nullptr, propertyCount, properties);
            if (found != nullptr && found->Type == DEVPROP_TYPE_STRING && found->Buffer != nullptr)
            {
                return reinterpret_cast<wchar_t const*>(found->Buffer);
            }

            return L"";
        }

        ContainerLabels ReadContainerLabels(GUID const& containerId)
        {
            ContainerLabels labels;
            const std::wstring objectId = GuidToBracedString(containerId);
            DEVPROPCOMPKEY requested[] = {
                {kDevPropContainerFriendlyName, DEVPROP_STORE_SYSTEM, nullptr},
                {kDevPropContainerModelName, DEVPROP_STORE_SYSTEM, nullptr},
            };

            ULONG propertyCount = 0;
            const DEVPROPERTY* properties = nullptr;
            const HRESULT hr = DevGetObjectProperties(
                DevObjectTypeDeviceContainer,
                objectId.c_str(),
                DevQueryFlagNone,
                static_cast<ULONG>(std::size(requested)),
                requested,
                &propertyCount,
                &properties);
            if (FAILED(hr) || propertyCount == 0 || properties == nullptr)
            {
                return labels;
            }

            labels.friendlyName = ReadContainerPropertyString(propertyCount, properties, kDevPropContainerFriendlyName);
            labels.modelName = ReadContainerPropertyString(propertyCount, properties, kDevPropContainerModelName);
            DevFreeObjectProperties(propertyCount, properties);
            return labels;
        }

        std::wstring ResolveDeviceContainerLabel(DEVINST devInst)
        {
            const auto containerId = ReadDevNodeGuidProperty(devInst, kDevPropContainerId);
            if (!containerId.has_value())
            {
                return L"";
            }

            const ContainerLabels labels = ReadContainerLabels(*containerId);
            if (!labels.friendlyName.empty())
            {
                return labels.friendlyName;
            }

            return labels.modelName;
        }

        std::wstring ResolveBestDeviceLabel(
            HDEVINFO deviceInfoSet,
            SP_DEVINFO_DATA const& deviceInfo,
            SP_DEVICE_INTERFACE_DATA const& interfaceData)
        {
            struct LabelSource
            {
                std::wstring label;
                int rank;
            };

            std::vector<LabelSource> sources;

            auto consider = [&](std::wstring label, int rank)
            {
                if (label.empty() || IsInfrastructureLabel(label))
                {
                    return;
                }

                sources.push_back({std::move(label), rank});
            };

            constexpr int kContainerFriendly = -20;
            constexpr int kBusReported = 0;
            constexpr int kFriendlyName = 1;
            constexpr int kInterfaceFriendly = 2;
            constexpr int kRegistryFriendly = 3;
            constexpr int kParentOffset = 10;

            consider(ResolveDeviceContainerLabel(deviceInfo.DevInst), kContainerFriendly);

            consider(
                ReadDevicePropertyString(deviceInfoSet, deviceInfo, kDevPropBusReportedDeviceDesc),
                kBusReported);
            consider(
                ReadDevicePropertyString(deviceInfoSet, deviceInfo, kDevPropDeviceFriendlyName),
                kFriendlyName);
            consider(
                ReadDeviceInterfacePropertyString(
                    deviceInfoSet, interfaceData, kDevPropDeviceInterfaceFriendlyName),
                kInterfaceFriendly);
            consider(ReadDeviceString(deviceInfoSet, deviceInfo, SPDRP_FRIENDLYNAME), kRegistryFriendly);

            DEVINST parent = 0;
            if (CM_Get_Parent(&parent, deviceInfo.DevInst, 0) == CR_SUCCESS && parent != 0)
            {
                consider(
                    ReadDevNodePropertyString(parent, kDevPropBusReportedDeviceDesc),
                    kParentOffset + kBusReported);
                consider(
                    ReadDevNodeRegistryProperty(parent, CM_DRP_FRIENDLYNAME),
                    kParentOffset + kRegistryFriendly);
            }

            std::sort(
                sources.begin(),
                sources.end(),
                [](LabelSource const& left, LabelSource const& right) { return left.rank < right.rank; });

            for (auto const& source : sources)
            {
                if (!IsGenericHidLabel(source.label))
                {
                    return source.label;
                }
            }

            if (!sources.empty())
            {
                return sources.front().label;
            }

            return L"HID Device";
        }

        void MergeDeviceLabel(HidDeviceInfo& existing, HidDeviceInfo const& incoming)
        {
            const bool existingGeneric = IsGenericHidLabel(existing.label);
            const bool incomingGeneric = IsGenericHidLabel(incoming.label);
            if (existingGeneric && !incomingGeneric)
            {
                existing.label = incoming.label;
            }
            else if (!existingGeneric && !incomingGeneric && incoming.label.size() > existing.label.size())
            {
                existing.label = incoming.label;
            }
        }
    }

    bool HidDevicesMatch(MonitoredHidDevice const& monitored, HidDeviceInfo const& online)
    {
        return ToUpperHex(monitored.vid) == ToUpperHex(online.vid) &&
            ToUpperHex(monitored.pid) == ToUpperHex(online.pid);
    }

    bool HidDeviceEnumerationService::ParseVidPid(
        std::wstring const& devicePath,
        std::wstring& vid,
        std::wstring& pid)
    {
        std::wstring normalized = devicePath;
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::towupper);

        const auto vidPos = normalized.find(L"VID_");
        const auto pidPos = normalized.find(L"PID_");
        if (vidPos == std::wstring::npos || pidPos == std::wstring::npos)
        {
            return false;
        }

        vid = normalized.substr(vidPos + 4, 4);
        pid = normalized.substr(pidPos + 4, 4);
        return vid.size() == 4 && pid.size() == 4;
    }

    bool HidDeviceEnumerationService::IsInternalTouchpad(
        std::wstring const& label,
        std::wstring const& hardwareId)
    {
        if (ContainsInsensitive(label, L"Touch Pad") ||
            ContainsInsensitive(label, L"Touchpad") ||
            ContainsInsensitive(label, L"Precision Touch") ||
            ContainsInsensitive(label, L"Trackpad"))
        {
            return true;
        }

        if (ContainsInsensitive(hardwareId, L"ACPI") &&
            (ContainsInsensitive(hardwareId, L"TPAD") ||
                ContainsInsensitive(hardwareId, L"TOUCHPAD") ||
                ContainsInsensitive(hardwareId, L"MSFT0001")))
        {
            return true;
        }

        return false;
    }

    std::vector<HidDeviceInfo> HidDeviceEnumerationService::ListConnectedHidDevices(
        bool excludeInternalTouchpad) const
    {
        std::vector<HidDeviceInfo> devices;

        HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(
            &GUID_DEVINTERFACE_HID,
            nullptr,
            nullptr,
            DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
        if (deviceInfoSet == INVALID_HANDLE_VALUE)
        {
            Logger::Error(L"SetupDiGetClassDevsW failed for HID");
            return devices;
        }

        SP_DEVICE_INTERFACE_DATA interfaceData{};
        interfaceData.cbSize = sizeof(interfaceData);

        for (DWORD index = 0;; ++index)
        {
            if (!SetupDiEnumDeviceInterfaces(deviceInfoSet, nullptr, &GUID_DEVINTERFACE_HID, index, &interfaceData))
            {
                break;
            }

            DWORD requiredSize = 0;
            SetupDiGetDeviceInterfaceDetailW(deviceInfoSet, &interfaceData, nullptr, 0, &requiredSize, nullptr);
            if (requiredSize == 0)
            {
                continue;
            }

            std::vector<BYTE> detailBuffer(requiredSize);
            auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(detailBuffer.data());
            detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

            SP_DEVINFO_DATA deviceInfo{};
            deviceInfo.cbSize = sizeof(deviceInfo);
            if (!SetupDiGetDeviceInterfaceDetailW(
                    deviceInfoSet,
                    &interfaceData,
                    detail,
                    requiredSize,
                    nullptr,
                    &deviceInfo))
            {
                continue;
            }

            HidDeviceInfo entry{};
            const std::wstring hardwareId = ReadDeviceString(deviceInfoSet, deviceInfo, SPDRP_HARDWAREID);

            const std::wstring devicePath = detail->DevicePath;
            if (!ParseVidPid(devicePath, entry.vid, entry.pid) &&
                !ParseVidPid(hardwareId, entry.vid, entry.pid))
            {
                continue;
            }

            entry.label = ResolveBestDeviceLabel(deviceInfoSet, deviceInfo, interfaceData);

            if (excludeInternalTouchpad && IsInternalTouchpad(entry.label, hardwareId))
            {
                continue;
            }

            const auto duplicate = std::find_if(
                devices.begin(),
                devices.end(),
                [&](HidDeviceInfo const& existing)
                {
                    return ToUpperHex(existing.vid) == ToUpperHex(entry.vid) &&
                        ToUpperHex(existing.pid) == ToUpperHex(entry.pid);
                });
            if (duplicate == devices.end())
            {
                devices.push_back(std::move(entry));
            }
            else
            {
                MergeDeviceLabel(*duplicate, entry);
            }
        }

        SetupDiDestroyDeviceInfoList(deviceInfoSet);

        return devices;
    }

    int HidDeviceEnumerationService::CountMonitoredOnline(
        std::vector<MonitoredHidDevice> const& monitored) const
    {
        if (monitored.empty())
        {
            return 0;
        }

        const auto onlineDevices = ListConnectedHidDevices(true);
        int count = 0;
        for (auto const& online : onlineDevices)
        {
            const bool matches = std::any_of(
                monitored.begin(),
                monitored.end(),
                [&](MonitoredHidDevice const& item) { return HidDevicesMatch(item, online); });
            if (matches)
            {
                ++count;
            }
        }
        return count;
    }
}
