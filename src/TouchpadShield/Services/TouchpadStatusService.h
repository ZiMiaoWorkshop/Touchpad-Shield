#pragma once

#include <optional>

namespace TouchpadShield::Services
{
    class TouchpadStatusService
    {
    public:
        std::optional<bool> IsEnabled() const;

    private:
        static constexpr wchar_t kStatusKeyPath[] =
            L"Software\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad\\Status";
        static constexpr wchar_t kEnabledValueName[] = L"Enabled";
    };
}
