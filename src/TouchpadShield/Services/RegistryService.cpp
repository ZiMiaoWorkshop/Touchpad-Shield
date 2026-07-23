#include "pch.h"
#include "Services/RegistryService.h"
#include "Services/RegistryUserContext.h"
#include "Services/TouchpadParametersService.h"
#include "Services/UnitConversion.h"

namespace TouchpadShield::Services
{
    namespace
    {
        constexpr REGSAM kRegistryWriteAccess = KEY_READ | KEY_WRITE | KEY_WOW64_64KEY;
        constexpr REGSAM kRegistryReadAccess = KEY_READ | KEY_WOW64_64KEY;

        bool IsHkcuRoot(HKEY root)
        {
            return root == HKEY_CURRENT_USER;
        }

        bool OpenKey(HKEY root, std::wstring const& subKey, REGSAM access, HKEY& keyOut)
        {
            return RegOpenKeyExW(root, subKey.c_str(), 0, access, &keyOut) == ERROR_SUCCESS;
        }

        bool OpenOrCreateKey(HKEY root, std::wstring const& subKey, HKEY& keyOut)
        {
            if (OpenKey(root, subKey, kRegistryWriteAccess, keyOut))
            {
                return true;
            }

            DWORD disposition = 0;
            const LSTATUS status = RegCreateKeyExW(
                root,
                subKey.c_str(),
                0,
                nullptr,
                REG_OPTION_NON_VOLATILE,
                kRegistryWriteAccess,
                nullptr,
                &keyOut,
                &disposition);

            if (status != ERROR_SUCCESS)
            {
                Logger::Error(L"Failed to open/create registry key " + subKey + L": " + std::to_wstring(status));
                return false;
            }

            return true;
        }
    }

    bool RegistryService::EnsureCurtainKeysExist()
    {
        static const std::array<std::wstring, 8> keys = {
            L"CurtainTop", L"CurtainLeft", L"CurtainRight", L"CurtainBottom",
            L"SuperCurtainTop", L"SuperCurtainLeft", L"SuperCurtainRight", L"SuperCurtainBottom"
        };

        bool ok = true;
        for (auto const& key : keys)
        {
            ok = EnsureDwordExists(HKEY_LOCAL_MACHINE, kPtpPath, key) && ok;
        }

        Logger::Info(L"Curtain registry keys ensured");
        return ok;
    }

    std::optional<uint32_t> RegistryService::APP_ClickForceSensitivity() const
    {
        if (auto value = TouchpadParametersService::GetClickForceSensitivity())
        {
            return value;
        }

        return ReadDword(HKEY_CURRENT_USER, kPtpPath, L"ClickForceSensitivity");
    }

    bool RegistryService::APP_SetClickForceSensitivity(uint32_t value)
    {
        value = std::min<uint32_t>(value, 100);
        if (TouchpadParametersService::SetClickForceSensitivity(value))
        {
            return true;
        }

        const bool ok = WriteDword(HKEY_CURRENT_USER, kPtpPath, L"ClickForceSensitivity", value);
        if (ok)
        {
            NotifyPrecisionTouchPadSettingsChanged();
        }
        return ok;
    }

    std::optional<uint32_t> RegistryService::APP_AAPThreshold() const
    {
        if (auto value = TouchpadParametersService::GetAAPThreshold())
        {
            return value;
        }

        return ReadDword(HKEY_CURRENT_USER, kPtpPath, L"AAPThreshold");
    }

    bool RegistryService::APP_SetAAPThreshold(uint32_t value)
    {
        value = std::min<uint32_t>(value, 3);
        if (TouchpadParametersService::SetAAPThreshold(value))
        {
            return true;
        }

        const bool ok = WriteDword(HKEY_CURRENT_USER, kPtpPath, L"AAPThreshold", value);
        if (ok)
        {
            NotifyPrecisionTouchPadSettingsChanged();
        }
        return ok;
    }

    std::optional<uint32_t> RegistryService::APP_TapsEnabled() const
    {
        if (auto enabled = TouchpadParametersService::GetTapsEnabled())
        {
            return *enabled ? 1u : 0u;
        }

        return ReadDword(HKEY_CURRENT_USER, kPtpPath, L"TapsEnabled");
    }

    bool RegistryService::APP_SetTapsEnabled(uint32_t value)
    {
        const bool enabled = value != 0;
        if (TouchpadParametersService::SetTapsEnabled(enabled))
        {
            return true;
        }

        value = enabled ? 1u : 0u;
        const bool ok = WriteDword(HKEY_CURRENT_USER, kPtpPath, L"TapsEnabled", value);
        if (ok)
        {
            NotifyPrecisionTouchPadSettingsChanged();
        }
        return ok;
    }

    EdgeValues RegistryService::APP_CurtainMm() const
    {
        return {
            ReadEdgeMm(HKEY_LOCAL_MACHINE, L"CurtainTop"),
            ReadEdgeMm(HKEY_LOCAL_MACHINE, L"CurtainBottom"),
            ReadEdgeMm(HKEY_LOCAL_MACHINE, L"CurtainLeft"),
            ReadEdgeMm(HKEY_LOCAL_MACHINE, L"CurtainRight")
        };
    }

    bool RegistryService::APP_SetCurtainMm(EdgeValues const& values)
    {
        bool ok = true;
        ok = WriteEdgeMm(HKEY_LOCAL_MACHINE, L"CurtainTop", values.topMm) && ok;
        ok = WriteEdgeMm(HKEY_LOCAL_MACHINE, L"CurtainBottom", values.bottomMm) && ok;
        ok = WriteEdgeMm(HKEY_LOCAL_MACHINE, L"CurtainLeft", values.leftMm) && ok;
        ok = WriteEdgeMm(HKEY_LOCAL_MACHINE, L"CurtainRight", values.rightMm) && ok;
        return ok;
    }

    EdgeValues RegistryService::APP_SuperCurtainMm() const
    {
        return {
            ReadEdgeMm(HKEY_LOCAL_MACHINE, L"SuperCurtainTop"),
            ReadEdgeMm(HKEY_LOCAL_MACHINE, L"SuperCurtainBottom"),
            ReadEdgeMm(HKEY_LOCAL_MACHINE, L"SuperCurtainLeft"),
            ReadEdgeMm(HKEY_LOCAL_MACHINE, L"SuperCurtainRight")
        };
    }

    bool RegistryService::APP_SetSuperCurtainMm(EdgeValues const& values)
    {
        bool ok = true;
        ok = WriteEdgeMm(HKEY_LOCAL_MACHINE, L"SuperCurtainTop", values.topMm) && ok;
        ok = WriteEdgeMm(HKEY_LOCAL_MACHINE, L"SuperCurtainBottom", values.bottomMm) && ok;
        ok = WriteEdgeMm(HKEY_LOCAL_MACHINE, L"SuperCurtainLeft", values.leftMm) && ok;
        ok = WriteEdgeMm(HKEY_LOCAL_MACHINE, L"SuperCurtainRight", values.rightMm) && ok;
        return ok;
    }

    std::optional<uint32_t> RegistryService::ReadDword(HKEY root, std::wstring const& subKey, std::wstring const& valueName) const
    {
        std::optional<uint32_t> result;

        auto readAction = [&]() -> bool
        {
            HKEY key = nullptr;
            if (!OpenKey(root, subKey, kRegistryReadAccess, key))
            {
                return false;
            }

            DWORD type = REG_DWORD;
            DWORD data = 0;
            DWORD size = sizeof(data);
            const LSTATUS status = RegQueryValueExW(key, valueName.c_str(), nullptr, &type, reinterpret_cast<LPBYTE>(&data), &size);
            RegCloseKey(key);

            if (status != ERROR_SUCCESS || type != REG_DWORD)
            {
                return false;
            }

            result = data;
            return true;
        };

        if (IsHkcuRoot(root))
        {
            RunAsInteractiveUser(readAction);
        }
        else
        {
            readAction();
        }

        return result;
    }

    bool RegistryService::WriteDword(HKEY root, std::wstring const& subKey, std::wstring const& valueName, uint32_t value) const
    {
        bool success = false;

        auto writeAction = [&]() -> bool
        {
            HKEY key = nullptr;
            if (!OpenOrCreateKey(root, subKey, key))
            {
                return false;
            }

            const DWORD data = value;
            const LSTATUS status = RegSetValueExW(
                key,
                valueName.c_str(),
                0,
                REG_DWORD,
                reinterpret_cast<const BYTE*>(&data),
                sizeof(data));

            if (status != ERROR_SUCCESS)
            {
                Logger::Error(L"Failed to write registry value " + valueName + L": " + std::to_wstring(status));
                RegCloseKey(key);
                return false;
            }

            RegFlushKey(key);
            RegCloseKey(key);
            Logger::Info(L"Wrote registry " + valueName + L"=" + std::to_wstring(value));
            return true;
        };

        if (IsHkcuRoot(root))
        {
            success = RunAsInteractiveUser(writeAction);
        }
        else
        {
            success = writeAction();
        }

        return success;
    }

    bool RegistryService::EnsureDwordExists(HKEY root, std::wstring const& subKey, std::wstring const& valueName)
    {
        if (ReadDword(root, subKey, valueName).has_value())
        {
            return true;
        }

        Logger::Info(L"Creating missing registry key " + valueName + L"=0");
        return WriteDword(root, subKey, valueName, 0);
    }

    double RegistryService::ReadEdgeMm(HKEY root, std::wstring const& valueName) const
    {
        const auto value = ReadDword(root, kPtpPath, valueName);
        if (!value.has_value())
        {
            return 0.0;
        }
        return HimetricToMm(value.value());
    }

    bool RegistryService::WriteEdgeMm(HKEY root, std::wstring const& valueName, double mm) const
    {
        if (mm < 0.0)
        {
            mm = 0.0;
        }
        return WriteDword(root, kPtpPath, valueName, MmToHimetric(mm));
    }
}
