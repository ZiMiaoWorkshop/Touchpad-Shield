#include "pch.h"
#include "Services/WindowBoundsHelper.h"

#include <winrt/Microsoft.UI.Interop.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Windows.Graphics.h>
#include <commctrl.h>

#pragma comment(lib, "Comctl32.lib")

namespace TouchpadShield::Services
{
    int WindowBoundsHelper::ScaleLogicalToPhysical(int logical, int dpi)
    {
        return MulDiv(logical, dpi, USER_DEFAULT_SCREEN_DPI);
    }

    void WindowBoundsHelper::Apply(HWND hwnd, WindowBoundsSpec const& spec)
    {
        if (!hwnd)
        {
            return;
        }

        m_spec = spec;
        SetWindowSubclass(hwnd, SubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));
    }

    void WindowBoundsHelper::ResizeClientToLogicalSize(HWND hwnd) const
    {
        if (!hwnd)
        {
            return;
        }

        const int dpi = GetDpiForWindow(hwnd);
        const int clientWidth = ScaleLogicalToPhysical(m_spec.logicalClientWidth, dpi);
        const int clientHeight = ScaleLogicalToPhysical(m_spec.logicalClientHeight, dpi);

        const auto windowId = winrt::Microsoft::UI::GetWindowIdFromWindow(hwnd);
        if (auto appWindow = winrt::Microsoft::UI::Windowing::AppWindow::GetFromWindowId(windowId))
        {
            appWindow.ResizeClient(winrt::Windows::Graphics::SizeInt32{
                static_cast<int32_t>(clientWidth),
                static_cast<int32_t>(clientHeight) });
        }
    }

    LRESULT CALLBACK WindowBoundsHelper::SubclassProc(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam,
        UINT_PTR /*idSubclass*/,
        DWORD_PTR refData)
    {
        auto* self = reinterpret_cast<WindowBoundsHelper*>(refData);
        if (msg == WM_GETMINMAXINFO && self)
        {
            const int dpi = GetDpiForWindow(hwnd);
            const int clientWidth = ScaleLogicalToPhysical(self->m_spec.logicalClientWidth, dpi);
            const int clientHeight = ScaleLogicalToPhysical(self->m_spec.logicalClientHeight, dpi);

            RECT rect{ 0, 0, clientWidth, clientHeight };
            const DWORD style = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_STYLE));
            const DWORD exStyle = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
            AdjustWindowRectEx(&rect, style, FALSE, exStyle);

            auto* minMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
            minMaxInfo->ptMinTrackSize.x = rect.right - rect.left;
            minMaxInfo->ptMinTrackSize.y = rect.bottom - rect.top;
        }

        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }
}
