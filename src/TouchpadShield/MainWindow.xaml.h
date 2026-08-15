#pragma once

#include "MainWindow.g.h"
#include "MainWindow.xaml.g.h"
#include "Services/AutoStartService.h"
#include "Services/BiosService.h"
#include "Services/CsvConfigService.h"
#include "Services/DisplayScaleService.h"
#include "Services/HidDeviceEnumerationService.h"
#include "Services/HidDeviceMonitorService.h"
#include "Services/HidDeviceTypes.h"
#include "Services/LocalSettingsService.h"
#include "Services/RegistryService.h"
#include "Services/TouchpadDiagramRenderer.h"
#include "Services/TrayIconService.h"
#include "Services/WindowBoundsHelper.h"

#include <memory>
#include <optional>
#include <vector>

namespace winrt::TouchpadShield::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        winrt::Windows::Foundation::IAsyncAction InitializeAsync();
        void CompletePlatformSetup();
        void LaunchToTrayOnly();
        void RequestExit();

        void RefreshButton_Click(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ClickSensitivitySlider_ValueChanged(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);
        void ClickModeComboBox_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);
        void TouchpadSensitivityComboBox_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);
        void TapsEnabledSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void SmartAreaSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void SmartEdgeSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void CurtainValueChanged(Microsoft::UI::Xaml::Controls::NumberBox const& sender, Microsoft::UI::Xaml::Controls::NumberBoxValueChangedEventArgs const& args);
        void SuperCurtainValueChanged(Microsoft::UI::Xaml::Controls::NumberBox const& sender, Microsoft::UI::Xaml::Controls::NumberBoxValueChangedEventArgs const& args);
        void TouchpadSizeValueChanged(Microsoft::UI::Xaml::Controls::NumberBox const& sender, Microsoft::UI::Xaml::Controls::NumberBoxValueChangedEventArgs const& args);
        void ApplyMatchedSizeButton_Click(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        winrt::Windows::Foundation::IAsyncAction ExportTouchpadSizeButton_Click(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void MoreTouchpadSettings_Click(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        winrt::Windows::Foundation::IAsyncAction RestartButton_Click(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void DiagramCanvas_SizeChanged(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::SizeChangedEventArgs const& args);
        void HidAutoTouchpadSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void HidRefreshButton_Click(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void RunAtStartupSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void MinimizeToTraySwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);

    private:
        ::TouchpadShield::Services::RegistryService m_registry{};
        ::TouchpadShield::Services::BiosService m_bios{};
        ::TouchpadShield::Services::LocalSettingsService m_localSettings{};
        ::TouchpadShield::Services::DisplayScaleService m_displayScale{};
        ::TouchpadShield::Services::WindowBoundsHelper m_windowBounds{};
        ::TouchpadShield::Services::TouchpadDiagramRenderer m_diagramRenderer{};
        ::TouchpadShield::Services::HidDeviceEnumerationService m_hidEnumeration{};
        ::TouchpadShield::Services::HidDeviceMonitorService m_hidMonitor{};
        ::TouchpadShield::Services::TrayIconService m_trayIcon{};
        ::TouchpadShield::Services::AutoStartService m_autoStart{};
        std::unique_ptr<::TouchpadShield::Services::CsvConfigService> m_csvService{};

        std::vector<::TouchpadShield::Services::HidDeviceInfo> m_availableHidDevices{};
        std::vector<::TouchpadShield::Services::MonitoredHidDevice> m_monitoredHidDevices{};

        ::TouchpadShield::Services::BiosIdentity m_biosIdentity{};
        std::optional<::TouchpadShield::Services::TouchpadPhysicalSizeEntry> m_matchedEntry{};
        ::TouchpadShield::Services::ClickSensitivityMode m_clickMode{ ::TouchpadShield::Services::ClickSensitivityMode::MatchWindowsSettings };
        bool m_isLoading{ false };
        bool m_isInitialized{ false };
        bool m_curtainRestartPending{ false };
        bool m_csvFileExists{ false };
        bool m_initialWindowSizeApplied{ false };
        bool m_forceExit{ false };
        bool m_hidSettingsLoading{ false };

        void InitializeWindow();
        void ApplyWindowIcon();
        void ApplyInitialWindowSize();
        void InitializeComboBoxes();
        void LoadAllData();
        void RefreshDiagram();
        void UpdateClickSensitivityLabel(int value);
        int SnapClickSensitivity(int value) const;
        std::wstring ClickSensitivityLabel(int value) const;
        ::TouchpadShield::Services::EdgeValues ReadCurtainFromUi();
        ::TouchpadShield::Services::EdgeValues ReadSuperCurtainFromUi();
        void SetCurtainUi(::TouchpadShield::Services::EdgeValues const& values, bool enabled);
        void SetSuperCurtainUi(::TouchpadShield::Services::EdgeValues const& values, bool enabled);
        void UpdateMatchedSizeUi();
        void UpdateExportButtonVisibility();
        void MarkCurtainRestartRequired();
        void UpdateRestartButtonVisibility();
        void PerformSystemReboot();
        std::filesystem::path ResolveCsvPath() const;

        void LoadHidSettingsUi();
        void RefreshAvailableHidDevices();
        void RefreshHidDeviceListsUi();
        void AddMonitoredHidDevice(::TouchpadShield::Services::HidDeviceInfo const& device);
        void RemoveMonitoredHidDevice(size_t index);
        void SaveMonitoredHidDevices();
        void ApplyHidPolicyLocks();
        void UpdateTrayIconState();
        void StartHidMonitoring();
        void StopHidMonitoring();
        bool ShouldMinimizeToTrayOnClose() const;
        void HideToTray();
        void ShowFromTray();
        HWND GetWindowHandle() const;
        void SetupWindowCloseBehavior();
    };
}

namespace winrt::TouchpadShield::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
