#pragma once

#include <optional>

namespace TouchpadShield::Services
{
    enum class ClickSensitivityMode
    {
        MatchWindowsSettings,
        FreeAdjust
    };

    struct SavedTouchpadSize
    {
        double widthMm{ 0.0 };
        double heightMm{ 0.0 };
        bool hasValue{ false };
    };

    class LocalSettingsService
    {
    public:
        SavedTouchpadSize LoadTouchpadSize() const;
        void SaveTouchpadSize(double widthMm, double heightMm);

        ClickSensitivityMode LoadClickSensitivityMode() const;
        void SaveClickSensitivityMode(ClickSensitivityMode mode);

    private:
        static constexpr wchar_t kAppKeyPath[] = L"Software\\ZiMiaoWorkshop\\TouchpadShield";

        std::optional<std::wstring> ReadStringValue(std::wstring const& name) const;
        bool WriteStringValue(std::wstring const& name, std::wstring const& value) const;
    };
}
