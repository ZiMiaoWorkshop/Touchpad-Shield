#pragma once

#include "Services/HidDeviceTypes.h"

#include <optional>
#include <vector>

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

        bool LoadBoolSetting(std::wstring const& name, bool defaultValue = false) const;
        void SaveBoolSetting(std::wstring const& name, bool value) const;

        bool LoadHidAutoTouchpadEnabled() const;
        void SaveHidAutoTouchpadEnabled(bool enabled) const;

        bool LoadRunAtStartup() const;
        void SaveRunAtStartup(bool enabled) const;

        bool LoadMinimizeToTrayOnClose() const;
        void SaveMinimizeToTrayOnClose(bool enabled) const;

        std::vector<MonitoredHidDevice> LoadMonitoredHidDevices() const;
        void SaveMonitoredHidDevices(std::vector<MonitoredHidDevice> const& devices) const;

    private:
        static constexpr wchar_t kAppKeyPath[] = L"Software\\ZiMiaoWorkshop\\TouchpadShield";

        std::optional<std::wstring> ReadStringValue(std::wstring const& name) const;
        bool WriteStringValue(std::wstring const& name, std::wstring const& value) const;

        static std::wstring SerializeMonitoredDevices(std::vector<MonitoredHidDevice> const& devices);
        static std::vector<MonitoredHidDevice> DeserializeMonitoredDevices(std::wstring const& json);
        static std::wstring ExtractJsonString(std::wstring const& json, std::wstring const& key, size_t& pos);
    };
}
