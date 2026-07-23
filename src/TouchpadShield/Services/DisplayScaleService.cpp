#include "pch.h"
#include "Services/DisplayScaleService.h"

#include <ShellScalingApi.h>
#pragma comment(lib, "Shcore.lib")

namespace TouchpadShield::Services
{
    namespace
    {
        DisplayMetrics BuildDisplayMetrics(HMONITOR monitor, double horizontalDpi)
        {
            DisplayMetrics metrics{};
            metrics.horizontalDpi = horizontalDpi;

            MONITORINFOEXW monitorInfo{};
            monitorInfo.cbSize = sizeof(monitorInfo);
            if (monitor && GetMonitorInfoW(monitor, &monitorInfo))
            {
                metrics.pixelWidth = static_cast<uint32_t>(monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left);
            }

            if (metrics.pixelWidth > 0 && horizontalDpi > 0.0)
            {
                metrics.widthMm = (static_cast<double>(metrics.pixelWidth) / horizontalDpi) * 25.4;
            }

            return metrics;
        }
    }

    DisplayMetrics DisplayScaleService::GetDisplayMetricsForWindow(HWND hwnd) const
    {
        const HMONITOR monitor = hwnd
            ? MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST)
            : MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);

        double horizontalDpi = 96.0;
        if (hwnd)
        {
            horizontalDpi = GetDpiForWindow(hwnd);
        }
        else if (monitor)
        {
            UINT dpiX = 96;
            UINT dpiY = 96;
            GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
            horizontalDpi = dpiX;
        }

        return BuildDisplayMetrics(monitor, horizontalDpi);
    }

    double DisplayScaleService::MillimetersPerPixel(DisplayMetrics const& metrics) const
    {
        if (metrics.pixelWidth == 0 || metrics.horizontalDpi <= 0.0)
        {
            return 25.4 / 96.0;
        }

        const double logicalWidthDip = static_cast<double>(metrics.pixelWidth) * 96.0 / metrics.horizontalDpi;
        if (logicalWidthDip <= 0.0 || metrics.widthMm <= 0.0)
        {
            return 25.4 / 96.0;
        }

        return metrics.widthMm / logicalWidthDip;
    }
}
