#pragma once

#include <functional>

namespace TouchpadShield::Services
{
    class TrayIconService
    {
    public:
        using ShowWindowCallback = std::function<void()>;
        using ExitCallback = std::function<void()>;

        bool Create(HWND hwnd);
        void Destroy();
        bool IsCreated() const { return m_created; }

        void SetShowWindowCallback(ShowWindowCallback callback);
        void SetExitCallback(ExitCallback callback);

        bool HandleWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    private:
        static constexpr UINT WM_TRAYICON = WM_APP + 100;
        static constexpr UINT_PTR kTraySubclassId = 3;
        static constexpr UINT kTrayIconId = 1;

        void AttachSubclass(HWND hwnd);
        void ShowContextMenu();
        HICON LoadAppIcon() const;
        static LRESULT CALLBACK SubclassProc(
            HWND hwnd,
            UINT msg,
            WPARAM wParam,
            LPARAM lParam,
            UINT_PTR idSubclass,
            DWORD_PTR refData);

        HWND m_hwnd{ nullptr };
        bool m_created{ false };
        ShowWindowCallback m_showWindow{};
        ExitCallback m_exit{};
    };
}
