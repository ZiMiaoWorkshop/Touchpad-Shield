#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace TouchpadShield::Services
{
    struct EdgeValues
    {
        double topMm{ 0.0 };
        double bottomMm{ 0.0 };
        double leftMm{ 0.0 };
        double rightMm{ 0.0 };
    };

    class RegistryService
    {
    public:
        bool EnsureCurtainKeysExist();

        std::optional<uint32_t> APP_ClickForceSensitivity() const;
        bool APP_SetClickForceSensitivity(uint32_t value);

        std::optional<uint32_t> APP_AAPThreshold() const;
        bool APP_SetAAPThreshold(uint32_t value);

        std::optional<uint32_t> APP_TapsEnabled() const;
        bool APP_SetTapsEnabled(uint32_t value);

        EdgeValues APP_CurtainMm() const;
        bool APP_SetCurtainMm(EdgeValues const& values);

        EdgeValues APP_SuperCurtainMm() const;
        bool APP_SetSuperCurtainMm(EdgeValues const& values);

    private:
        static constexpr wchar_t kPtpPath[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad";

        std::optional<uint32_t> ReadDword(HKEY root, std::wstring const& subKey, std::wstring const& valueName) const;
        bool WriteDword(HKEY root, std::wstring const& subKey, std::wstring const& valueName, uint32_t value) const;
        bool EnsureDwordExists(HKEY root, std::wstring const& subKey, std::wstring const& valueName);
        double ReadEdgeMm(HKEY root, std::wstring const& valueName) const;
        bool WriteEdgeMm(HKEY root, std::wstring const& valueName, double mm) const;
    };
}
