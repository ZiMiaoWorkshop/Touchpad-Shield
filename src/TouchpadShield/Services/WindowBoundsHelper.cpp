#include "pch.h"
#include "Services/WindowBoundsHelper.h"

#include <winrt/Microsoft.UI.Interop.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <commctrl.h>

#pragma comment(lib, "Comctl32.lib")

namespace TouchpadShield::Services
{
    int WindowBoundsHelper::ScaleLogicalToPhysical(int logical, int dpi)
    {
        return MulDiv(logical, dpi, USER_DEFAULT_SCREEN_DPI);
    }

    void WindowBoundsHelper::ComputePhysicalClientSize(
        HWND hwnd,
        WindowBoundsSpec const& spec,
        int& width,
        int& height)
    {
        width = 0;
        height = 0;
        if (!hwnd)
        {
            return;
        }

        const int dpi = GetDpiForWindow(hwnd);
        width = ScaleLogicalToPhysical(spec.logicalClientWidth, dpi);
        height = ScaleLogicalToPhysical(spec.logicalClientHeight, dpi);
    }

    void WindowBoundsHelper::ComputeOuterTrackSize(
        HWND hwnd,
        WindowBoundsSpec const& spec,
        int& width,
        int& height)
    {
        width = 0;
        height = 0;
        if (!hwnd)
        {
            return;
        }

        int clientWidth = 0;
        int clientHeight = 0;
        ComputePhysicalClientSize(hwnd, spec, clientWidth, clientHeight);

        RECT rect{ 0, 0, clientWidth, clientHeight };
        const DWORD style = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_STYLE));
        const DWORD exStyle = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
        AdjustWindowRectEx(&rect, style, FALSE, exStyle);

        width = rect.right - rect.left;
        height = rect.bottom - rect.top;
    }

    void WindowBoundsHelper::ApplyInitialClientBounds(HWND hwnd, WindowBoundsSpec const& spec)
    {
        if (!hwnd)
        {
            return;
        }

        m_spec = spec;
        SetWindowSubclass(hwnd, SubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));

        int clientWidth = 0;
        int clientHeight = 0;
        ComputePhysicalClientSize(hwnd, m_spec, clientWidth, clientHeight);

        const auto windowId = winrt::Microsoft::UI::GetWindowIdFromWindow(hwnd);
        if (auto appWindow = winrt::Microsoft::UI::Windowing::AppWindow::GetFromWindowId(windowId))
        {
            appWindow.ResizeClient(winrt::Windows::Graphics::SizeInt32{
                static_cast<int32_t>(clientWidth),
                static_cast<int32_t>(clientHeight) });
        }

        CenterOnWorkArea(hwnd);
    }

    void WindowBoundsHelper::CenterOnWorkArea(HWND hwnd)
    {
        if (!hwnd)
        {
            return;
        }

        const auto windowId = winrt::Microsoft::UI::GetWindowIdFromWindow(hwnd);
        const auto appWindow = winrt::Microsoft::UI::Windowing::AppWindow::GetFromWindowId(windowId);
        if (!appWindow)
        {
            return;
        }

        const auto displayArea = winrt::Microsoft::UI::Windowing::DisplayArea::GetFromWindowId(
            windowId,
            winrt::Microsoft::UI::Windowing::DisplayAreaFallback::Primary);
        const winrt::Windows::Graphics::RectInt32 work = displayArea.WorkArea();
        const winrt::Windows::Graphics::SizeInt32 size = appWindow.Size();

        int x = work.X + (work.Width - size.Width) / 2;
        int y = work.Y + (work.Height - size.Height) / 2;
        if (x < work.X)
        {
            x = work.X;
        }
        if (y < work.Y)
        {
            y = work.Y;
        }

        appWindow.Move({ x, y });
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
            int outerWidth = 0;
            int outerHeight = 0;
            ComputeOuterTrackSize(hwnd, self->m_spec, outerWidth, outerHeight);

            auto* minMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
            minMaxInfo->ptMinTrackSize.x = outerWidth;
            minMaxInfo->ptMinTrackSize.y = outerHeight;
        }

        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }
}
