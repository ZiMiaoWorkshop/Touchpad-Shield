#include "pch.h"
#include "Services/LocalSettingsService.h"
#include "Services/UnitConversion.h"

namespace TouchpadShield::Services
{
    SavedTouchpadSize LocalSettingsService::LoadTouchpadSize() const
    {
        SavedTouchpadSize size{};
        const auto width = ReadStringValue(L"TouchpadWidthMm");
        const auto height = ReadStringValue(L"TouchpadHeightMm");
        if (!width.has_value() || !height.has_value())
        {
            return size;
        }

        size.widthMm = wcstod(width->c_str(), nullptr);
        size.heightMm = wcstod(height->c_str(), nullptr);
        size.hasValue = size.widthMm > 0.0 && size.heightMm > 0.0;
        return size;
    }

    void LocalSettingsService::SaveTouchpadSize(double widthMm, double heightMm)
    {
        WriteStringValue(L"TouchpadWidthMm", FormatMm(widthMm));
        WriteStringValue(L"TouchpadHeightMm", FormatMm(heightMm));
    }

    ClickSensitivityMode LocalSettingsService::LoadClickSensitivityMode() const
    {
        const auto mode = ReadStringValue(L"ClickSensitivityMode");
        if (mode.has_value() && mode.value() == L"FreeAdjust")
        {
            return ClickSensitivityMode::FreeAdjust;
        }
        return ClickSensitivityMode::MatchWindowsSettings;
    }

    void LocalSettingsService::SaveClickSensitivityMode(ClickSensitivityMode mode)
    {
        WriteStringValue(
            L"ClickSensitivityMode",
            mode == ClickSensitivityMode::FreeAdjust ? L"FreeAdjust" : L"MatchWindowsSettings");
    }

    bool LocalSettingsService::LoadBoolSetting(std::wstring const& name, bool defaultValue) const
    {
        const auto value = ReadStringValue(name);
        if (!value.has_value())
        {
            return defaultValue;
        }
        return value.value() == L"1";
    }

    void LocalSettingsService::SaveBoolSetting(std::wstring const& name, bool value) const
    {
        WriteStringValue(name, value ? L"1" : L"0");
    }

    bool LocalSettingsService::LoadInputAutoTouchpadEnabled() const
    {
        return LoadBoolSetting(kInputAutoTouchpadEnabledKey, false);
    }

    void LocalSettingsService::SaveInputAutoTouchpadEnabled(bool enabled) const
    {
        SaveBoolSetting(kInputAutoTouchpadEnabledKey, enabled);
    }

    bool LocalSettingsService::LoadRunAtStartup() const
    {
        return LoadBoolSetting(L"RunAtStartup", false);
    }

    void LocalSettingsService::SaveRunAtStartup(bool enabled) const
    {
        SaveBoolSetting(L"RunAtStartup", enabled);
    }

    bool LocalSettingsService::LoadMinimizeToTrayOnClose() const
    {
        return LoadBoolSetting(L"MinimizeToTrayOnClose", false);
    }

    void LocalSettingsService::SaveMinimizeToTrayOnClose(bool enabled) const
    {
        SaveBoolSetting(L"MinimizeToTrayOnClose", enabled);
    }

    std::vector<MonitoredInputDevice> LocalSettingsService::LoadMonitoredInputDevices() const
    {
        const auto json = ReadStringValue(kMonitoredInputDevicesKey);
        if (!json.has_value() || json->empty())
        {
            return {};
        }

        return DeserializeMonitoredDevices(json.value());
    }

    void LocalSettingsService::SaveMonitoredInputDevices(std::vector<MonitoredInputDevice> const& devices) const
    {
        WriteStringValue(kMonitoredInputDevicesKey, SerializeMonitoredDevices(devices));
    }

    std::wstring LocalSettingsService::SerializeMonitoredDevices(std::vector<MonitoredInputDevice> const& devices)
    {
        std::wstring json = L"[";
        for (size_t i = 0; i < devices.size(); ++i)
        {
            if (i > 0)
            {
                json += L",";
            }

            auto escape = [](std::wstring const& text)
            {
                std::wstring escaped;
                escaped.reserve(text.size());
                for (wchar_t ch : text)
                {
                    if (ch == L'\\' || ch == L'"')
                    {
                        escaped += L'\\';
                    }
                    escaped += ch;
                }
                return escaped;
            };

            json += L"{\"containerId\":\"" + escape(ContainerIdToString(devices[i].containerId)) +
                L"\",\"label\":\"" + escape(devices[i].label) + L"\"";
            if (!devices[i].matchKey.empty())
            {
                json += L",\"matchKey\":\"" + escape(devices[i].matchKey) + L"\"";
            }
            json += L"}";
        }
        json += L"]";
        return json;
    }

    std::wstring LocalSettingsService::ExtractJsonString(
        std::wstring const& json,
        std::wstring const& key,
        size_t& pos)
    {
        const std::wstring token = L"\"" + key + L"\":\"";
        const size_t start = json.find(token, pos);
        if (start == std::wstring::npos)
        {
            return L"";
        }

        size_t cursor = start + token.size();
        std::wstring value;
        while (cursor < json.size())
        {
            const wchar_t ch = json[cursor++];
            if (ch == L'\\' && cursor < json.size())
            {
                value += json[cursor++];
                continue;
            }
            if (ch == L'"')
            {
                break;
            }
            value += ch;
        }

        pos = cursor;
        return value;
    }

    std::vector<MonitoredInputDevice> LocalSettingsService::DeserializeMonitoredDevices(std::wstring const& json)
    {
        std::vector<MonitoredInputDevice> devices;
        size_t pos = 0;
        while (pos < json.size())
        {
            const size_t objectStart = json.find(L'{', pos);
            if (objectStart == std::wstring::npos)
            {
                break;
            }

            pos = objectStart + 1;
            MonitoredInputDevice device{};
            const std::wstring containerIdText = ExtractJsonString(json, L"containerId", pos);
            device.label = ExtractJsonString(json, L"label", pos);
            device.matchKey = ExtractJsonString(json, L"matchKey", pos);
            if (!containerIdText.empty() && TryParseContainerId(containerIdText, device.containerId))
            {
                devices.push_back(std::move(device));
            }

            const size_t objectEnd = json.find(L'}', pos);
            if (objectEnd == std::wstring::npos)
            {
                break;
            }
            pos = objectEnd + 1;
        }
        return devices;
    }

    std::optional<std::wstring> LocalSettingsService::ReadStringValue(std::wstring const& name) const
    {
        HKEY key = nullptr;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, kAppKeyPath, 0, KEY_READ, &key) != ERROR_SUCCESS)
        {
            return std::nullopt;
        }

        DWORD type = REG_SZ;
        DWORD size = 0;
        if (RegQueryValueExW(key, name.c_str(), nullptr, &type, nullptr, &size) != ERROR_SUCCESS || size == 0)
        {
            RegCloseKey(key);
            return std::nullopt;
        }

        std::vector<wchar_t> buffer(size / sizeof(wchar_t) + 1, L'\0');
        if (RegQueryValueExW(key, name.c_str(), nullptr, &type, reinterpret_cast<LPBYTE>(buffer.data()), &size) != ERROR_SUCCESS)
        {
            RegCloseKey(key);
            return std::nullopt;
        }

        RegCloseKey(key);
        return std::wstring(buffer.data());
    }

    bool LocalSettingsService::WriteStringValue(std::wstring const& name, std::wstring const& value) const
    {
        HKEY key = nullptr;
        DWORD disposition = 0;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, kAppKeyPath, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, &disposition) != ERROR_SUCCESS)
        {
            return false;
        }

        const LSTATUS status = RegSetValueExW(
            key,
            name.c_str(),
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(value.c_str()),
            static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(key);
        return status == ERROR_SUCCESS;
    }
}
