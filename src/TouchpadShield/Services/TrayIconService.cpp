#include "pch.h"
#include "Services/TrayIconService.h"

#include <commctrl.h>
#include <shellapi.h>

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Shell32.lib")

namespace TouchpadShield::Services
{
    void TrayIconService::SetShowWindowCallback(ShowWindowCallback callback)
    {
        m_showWindow = std::move(callback);
    }

    void TrayIconService::SetExitCallback(ExitCallback callback)
    {
        m_exit = std::move(callback);
    }

    HICON TrayIconService::LoadAppIcon() const
    {
        wchar_t modulePath[MAX_PATH]{};
        GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
        const auto iconPath = std::filesystem::path(modulePath).parent_path() / L"Assets" / L"TouchpadShield.ico";
        if (std::filesystem::exists(iconPath))
        {
            return static_cast<HICON>(LoadImageW(
                nullptr,
                iconPath.c_str(),
                IMAGE_ICON,
                0,
                0,
                LR_LOADFROMFILE | LR_DEFAULTSIZE));
        }

        return LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(1));
    }

    void TrayIconService::AttachSubclass(HWND hwnd)
    {
        m_hwnd = hwnd;
        SetWindowSubclass(hwnd, SubclassProc, kTraySubclassId, reinterpret_cast<DWORD_PTR>(this));
    }

    bool TrayIconService::Create(HWND hwnd)
    {
        if (!hwnd || m_created)
        {
            return false;
        }

        AttachSubclass(hwnd);

        NOTIFYICONDATAW data{};
        data.cbSize = sizeof(data);
        data.hWnd = hwnd;
        data.uID = kTrayIconId;
        data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        data.uCallbackMessage = WM_TRAYICON;
        data.hIcon = LoadAppIcon();
        wcscpy_s(data.szTip, L"Touchpad Shield");

        if (!Shell_NotifyIconW(NIM_ADD, &data))
        {
            Logger::Error(L"Shell_NotifyIconW NIM_ADD failed");
            return false;
        }

        m_created = true;
        Logger::Info(L"Tray icon created");
        return true;
    }

    void TrayIconService::Destroy()
    {
        if (m_created && m_hwnd)
        {
            NOTIFYICONDATAW data{};
            data.cbSize = sizeof(data);
            data.hWnd = m_hwnd;
            data.uID = kTrayIconId;
            Shell_NotifyIconW(NIM_DELETE, &data);
            m_created = false;
            Logger::Info(L"Tray icon destroyed");
        }

        if (m_hwnd)
        {
            RemoveWindowSubclass(m_hwnd, SubclassProc, kTraySubclassId);
            m_hwnd = nullptr;
        }
    }

    void TrayIconService::ShowContextMenu()
    {
        if (!m_hwnd)
        {
            return;
        }

        HMENU menu = CreatePopupMenu();
        AppendMenuW(menu, MF_STRING, 1, L"打开主窗口");
        AppendMenuW(menu, MF_STRING, 2, L"退出");

        POINT point{};
        GetCursorPos(&point);
        SetForegroundWindow(m_hwnd);

        const UINT command = TrackPopupMenu(
            menu,
            TPM_RETURNCMD | TPM_RIGHTBUTTON,
            point.x,
            point.y,
            0,
            m_hwnd,
            nullptr);

        DestroyMenu(menu);

        if (command == 1 && m_showWindow)
        {
            m_showWindow();
        }
        else if (command == 2 && m_exit)
        {
            m_exit();
        }
    }

    bool TrayIconService::HandleWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_TRAYICON)
        {
            if (LOWORD(lParam) == WM_LBUTTONUP && m_showWindow)
            {
                m_showWindow();
                return true;
            }

            if (LOWORD(lParam) == WM_RBUTTONUP)
            {
                ShowContextMenu();
                return true;
            }
        }

        return false;
    }

    LRESULT CALLBACK TrayIconService::SubclassProc(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam,
        UINT_PTR /*idSubclass*/,
        DWORD_PTR refData)
    {
        auto* self = reinterpret_cast<TrayIconService*>(refData);
        if (self && self->HandleWindowMessage(msg, wParam, lParam))
        {
            return TRUE;
        }

        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }
}
