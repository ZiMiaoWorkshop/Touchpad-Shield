#include "pch.h"
#include "Services/RegistryUserContext.h"

#include <wtsapi32.h>

namespace TouchpadShield::Services
{
    namespace
    {
        bool IsProcessElevated()
        {
            HANDLE token = nullptr;
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
            {
                return false;
            }

            TOKEN_ELEVATION elevation{};
            DWORD size = sizeof(elevation);
            const bool elevated = GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size) != FALSE
                && elevation.TokenIsElevated != 0;
            CloseHandle(token);
            return elevated;
        }

        bool TryImpersonateInteractiveUser(HANDLE& tokenOut)
        {
            tokenOut = nullptr;
            const DWORD sessionId = WTSGetActiveConsoleSessionId();
            if (sessionId == 0xFFFFFFFF)
            {
                return false;
            }

            if (!WTSQueryUserToken(sessionId, &tokenOut))
            {
                Logger::Error(L"WTSQueryUserToken failed: " + std::to_wstring(GetLastError()));
                return false;
            }

            if (!ImpersonateLoggedOnUser(tokenOut))
            {
                Logger::Error(L"ImpersonateLoggedOnUser failed: " + std::to_wstring(GetLastError()));
                CloseHandle(tokenOut);
                tokenOut = nullptr;
                return false;
            }

            return true;
        }
    }

    bool RunAsInteractiveUser(std::function<bool()> const& action)
    {
        if (!IsProcessElevated())
        {
            return action();
        }

        HANDLE userToken = nullptr;
        if (!TryImpersonateInteractiveUser(userToken))
        {
            Logger::Info(L"Falling back to current HKCU context for registry access");
            return action();
        }

        const bool result = action();
        RevertToSelf();
        CloseHandle(userToken);
        return result;
    }

    void NotifyPrecisionTouchPadSettingsChanged()
    {
        SendMessageTimeoutW(
            HWND_BROADCAST,
            WM_SETTINGCHANGE,
            0,
            reinterpret_cast<LPARAM>(const_cast<wchar_t*>(L"PrecisionTouchPad")),
            SMTO_ABORTIFHUNG,
            2000,
            nullptr);
    }
}
