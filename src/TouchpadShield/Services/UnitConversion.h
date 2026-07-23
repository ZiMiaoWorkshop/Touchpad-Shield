#pragma once

#include <cstdint>
#include <string>

namespace TouchpadShield::Services
{
    constexpr double kDefaultTouchpadWidthMm = 65.0;
    constexpr double kDefaultTouchpadHeightMm = 40.0;
    constexpr double kHimetricPerMm = 100.0;

    uint32_t MmToHimetric(double mm);
    double HimetricToMm(uint32_t himetric);
    std::wstring FormatMm(double mm);
    bool SameTouchpadSizeMm(double widthA, double heightA, double widthB, double heightB);
}
