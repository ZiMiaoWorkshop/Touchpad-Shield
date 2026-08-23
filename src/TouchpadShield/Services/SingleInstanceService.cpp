#include "pch.h"
#include "Services/SingleInstanceService.h"

namespace TouchpadShield::Services
{
    namespace
    {
        BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
        {
            // Hidden tray windows may not be visible; still match by title below.
            wchar_t title[256]{};
            const int length = GetWindowTextW(hwnd, title, static_cast<int>(std::size(title)));
            if (length <= 0)
            {
                return TRUE;
            }

            if (_wcsicmp(title, kMainWindowTitle) != 0)
            {
                return TRUE;
            }

            auto* found = reinterpret_cast<HWND*>(lParam);
            *found = hwnd;
            return FALSE;
        }
    }

    bool SingleInstanceService::TryAcquire()
    {
        m_mutexHandle = CreateMutexW(nullptr, TRUE, kMutexName);
        if (!m_mutexHandle)
        {
            return false;
        }

        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            CloseHandle(m_mutexHandle);
            m_mutexHandle = nullptr;
            return false;
        }

        return true;
    }

    void SingleInstanceService::Release()
    {
        if (m_mutexHandle)
        {
            ReleaseMutex(m_mutexHandle);
            CloseHandle(m_mutexHandle);
            m_mutexHandle = nullptr;
        }
    }

    UINT SingleInstanceService::ActivateMainWindowMessage()
    {
        static const UINT message = RegisterWindowMessageW(L"TouchpadShield.ShowMainWindow");
        return message;
    }

    bool SingleInstanceService::ActivateExistingInstance()
    {
        HWND target = nullptr;
        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&target));
        if (!target)
        {
            return false;
        }

        const UINT showMessage = ActivateMainWindowMessage();
        if (showMessage == 0)
        {
            return false;
        }

        return PostMessageW(target, showMessage, 0, 0) != FALSE;
    }
}
