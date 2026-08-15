#include "pch.h"
#include "Services/AutoStartService.h"

namespace TouchpadShield::Services
{
    std::wstring AutoStartService::ResolveExecutablePath() const
    {
        wchar_t modulePath[MAX_PATH]{};
        const DWORD length = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
        if (length == 0 || length >= MAX_PATH)
        {
            return L"";
        }
        return std::wstring(L"\"") + modulePath + L"\"";
    }

    std::wstring AutoStartService::BuildRunCommand() const
    {
        const std::wstring exe = ResolveExecutablePath();
        if (exe.empty())
        {
            return L"";
        }
        return exe + L" " + kStartupArgument;
    }

    bool AutoStartService::IsStartupLaunch()
    {
        int argc = 0;
        wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        if (!argv)
        {
            return false;
        }

        bool isStartup = false;
        for (int i = 1; i < argc; ++i)
        {
            if (_wcsicmp(argv[i], kStartupArgument) == 0)
            {
                isStartup = true;
                break;
            }
        }

        LocalFree(argv);
        return isStartup;
    }

    bool AutoStartService::IsEnabled() const
    {
        HKEY key = nullptr;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKeyPath, 0, KEY_READ, &key) != ERROR_SUCCESS)
        {
            return false;
        }

        DWORD type = REG_SZ;
        DWORD size = 0;
        if (RegQueryValueExW(key, kRunValueName, nullptr, &type, nullptr, &size) != ERROR_SUCCESS || size == 0)
        {
            RegCloseKey(key);
            return false;
        }

        std::vector<wchar_t> buffer(size / sizeof(wchar_t) + 1, L'\0');
        const LSTATUS status = RegQueryValueExW(
            key,
            kRunValueName,
            nullptr,
            &type,
            reinterpret_cast<LPBYTE>(buffer.data()),
            &size);
        RegCloseKey(key);

        if (status != ERROR_SUCCESS)
        {
            return false;
        }

        const std::wstring expected = BuildRunCommand();
        const std::wstring legacy = ResolveExecutablePath();
        std::wstring actual = buffer.data();
        if (!expected.empty() && _wcsicmp(actual.c_str(), expected.c_str()) == 0)
        {
            return true;
        }
        return !legacy.empty() && _wcsicmp(actual.c_str(), legacy.c_str()) == 0;
    }

    bool AutoStartService::SetEnabled(bool enabled) const
    {
        HKEY key = nullptr;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKeyPath, 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS)
        {
            return false;
        }

        LSTATUS status = ERROR_SUCCESS;
        if (enabled)
        {
            const std::wstring command = BuildRunCommand();
            if (command.empty())
            {
                RegCloseKey(key);
                return false;
            }

            status = RegSetValueExW(
                key,
                kRunValueName,
                0,
                REG_SZ,
                reinterpret_cast<const BYTE*>(command.c_str()),
                static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
        }
        else
        {
            status = RegDeleteValueW(key, kRunValueName);
            if (status == ERROR_FILE_NOT_FOUND)
            {
                status = ERROR_SUCCESS;
            }
        }

        RegCloseKey(key);
        return status == ERROR_SUCCESS;
    }
}
