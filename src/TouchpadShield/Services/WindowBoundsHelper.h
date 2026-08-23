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
        void ApplyInitialClientBounds(HWND hwnd, WindowBoundsSpec const& spec = {});
        static void CenterOnWorkArea(HWND hwnd);

    private:
        WindowBoundsSpec m_spec{};
        static int ScaleLogicalToPhysical(int logical, int dpi);
        static void ComputePhysicalClientSize(HWND hwnd, WindowBoundsSpec const& spec, int& width, int& height);
        static void ComputeOuterTrackSize(HWND hwnd, WindowBoundsSpec const& spec, int& width, int& height);
        static LRESULT CALLBACK SubclassProc(
            HWND hwnd,
            UINT msg,
            WPARAM wParam,
            LPARAM lParam,
            UINT_PTR idSubclass,
            DWORD_PTR refData);
    };
}
