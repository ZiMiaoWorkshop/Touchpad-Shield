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
