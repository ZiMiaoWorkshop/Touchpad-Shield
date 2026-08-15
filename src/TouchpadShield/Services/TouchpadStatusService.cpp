#include "pch.h"
#include "Services/TouchpadStatusService.h"

namespace TouchpadShield::Services
{
    std::optional<bool> TouchpadStatusService::IsEnabled() const
    {
        HKEY key = nullptr;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, kStatusKeyPath, 0, KEY_READ, &key) != ERROR_SUCCESS)
        {
            return std::nullopt;
        }

        DWORD type = REG_DWORD;
        DWORD value = 0;
        DWORD size = sizeof(value);
        const LSTATUS status = RegQueryValueExW(
            key,
            kEnabledValueName,
            nullptr,
            &type,
            reinterpret_cast<LPBYTE>(&value),
            &size);
        RegCloseKey(key);

        if (status != ERROR_SUCCESS || type != REG_DWORD)
        {
            return std::nullopt;
        }

        return value != 0;
    }
}
