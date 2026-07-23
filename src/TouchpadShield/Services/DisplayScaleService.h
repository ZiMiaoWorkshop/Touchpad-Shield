#pragma once

#include <Windows.h>

namespace TouchpadShield::Services
{
    struct DisplayMetrics
    {
        double widthMm{ 0.0 };
        double horizontalDpi{ 96.0 };
        uint32_t pixelWidth{ 0 };
    };

    class DisplayScaleService
    {
    public:
        DisplayMetrics GetDisplayMetricsForWindow(HWND hwnd) const;
        double MillimetersPerPixel(DisplayMetrics const& metrics) const;
    };
}
