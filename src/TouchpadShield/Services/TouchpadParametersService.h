#pragma once

#include <cstdint>
#include <optional>

namespace TouchpadShield::Services
{
    class TouchpadParametersService
    {
    public:
        static std::optional<uint32_t> GetClickForceSensitivity();
        static std::optional<uint32_t> GetAAPThreshold();
        static std::optional<bool> GetTapsEnabled();

        static bool SetClickForceSensitivity(uint32_t value);
        static bool SetAAPThreshold(uint32_t value);
        static bool SetTapsEnabled(bool enabled);
    };
}
