#pragma once

#include <Windows.h>

namespace TouchpadShield::Services
{
    struct WindowBoundsSpec
    {
        int logicalClientWidth{ 1280 };
        int logicalClientHeight{ 900 };
    };

    class WindowBoundsHelper
    {
    public:
        void Apply(HWND hwnd, WindowBoundsSpec const& spec);
        void ResizeClientToLogicalSize(HWND hwnd) const;

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
