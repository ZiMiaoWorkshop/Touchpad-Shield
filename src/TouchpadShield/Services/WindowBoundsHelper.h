#pragma once

#include <Windows.h>

namespace TouchpadShield::Services
{
    inline constexpr int kDefaultLogicalClientWidth = 1560;
    inline constexpr int kDefaultLogicalClientHeight = 900;

    struct WindowBoundsSpec
    {
        int logicalClientWidth{ kDefaultLogicalClientWidth };
        int logicalClientHeight{ kDefaultLogicalClientHeight };
    };
    class WindowBoundsHelper
    {
    public:
        void Apply(HWND hwnd, WindowBoundsSpec const& spec);
        void ResizeClientToLogicalSize(HWND hwnd) const;
        static void CenterOnWorkArea(HWND hwnd);

    private:
        WindowBoundsSpec m_spec{};
        static int ScaleLogicalToPhysical(int logical, int dpi);
        static LRESULT CALLBACK SubclassProc(
            HWND hwnd,
            UINT msg,
            WPARAM wParam,
            LPARAM lParam,
            UINT_PTR idSubclass,
            DWORD_PTR refData);
    };
}
