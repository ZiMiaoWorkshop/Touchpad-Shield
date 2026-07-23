#include "pch.h"
#include "Services/UnitConversion.h"

namespace TouchpadShield::Services
{
    uint32_t MmToHimetric(double mm)
    {
        if (mm < 0.0)
        {
            mm = 0.0;
        }
        return static_cast<uint32_t>(std::llround(mm * kHimetricPerMm));
    }

    double HimetricToMm(uint32_t himetric)
    {
        return static_cast<double>(himetric) / kHimetricPerMm;
    }

    std::wstring FormatMm(double mm)
    {
        std::wostringstream stream;
        stream << std::fixed << std::setprecision(2) << mm;
        return stream.str();
    }

    bool SameTouchpadSizeMm(double widthA, double heightA, double widthB, double heightB)
    {
        return std::llround(widthA * 100.0) == std::llround(widthB * 100.0) &&
            std::llround(heightA * 100.0) == std::llround(heightB * 100.0);
    }
}
