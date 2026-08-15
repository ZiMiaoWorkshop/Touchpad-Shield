#include "pch.h"
#include "MainWindow.xaml.h"
#include "Services/UnitConversion.h"
#include "Services/WindowIconHelper.h"

#include <chrono>

#include <microsoft.ui.xaml.window.h>
#include <winrt/Microsoft.UI.Interop.h>
#include <winrt/Microsoft.UI.Windowing.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Windowing;
namespace AppServices = ::TouchpadShield::Services;

namespace winrt::TouchpadShield::implementation
{
    namespace
    {
        constexpr int kMinWindowWidth = 1560;
        constexpr int kMinWindowHeight = 900;
        constexpr double kOfflineHidDeviceLabelOpacity = 0.55;
        constexpr double kHidDeviceRowIndent = 16.0;

        bool IsMonitoredDeviceOnline(
            AppServices::MonitoredHidDevice const& monitored,
            std::vector<AppServices::HidDeviceInfo> const& onlineDevices)
        {
            return std::any_of(
                onlineDevices.begin(),
                onlineDevices.end(),
                [&](AppServices::HidDeviceInfo const& online)
                {
                    return AppServices::HidDevicesMatch(monitored, online);
                });
        }

        std::wstring BuildVersionText()
        {
#ifndef TOUCHPAD_SHIELD_VERSION
#define TOUCHPAD_SHIELD_VERSION L"0.0.0 build 0000"
#endif
            return std::wstring(L"v") + TOUCHPAD_SHIELD_VERSION;
        }

        double NumberBoxValueOrZero(NumberBox const& box)
        {
            const double value = box.Value();
            return std::isnan(value) ? 0.0 : value;
        }

        std::wstring StripVidPidSuffix(std::wstring label)
        {
            const auto suffixPos = label.rfind(L" (VID_");
            if (suffixPos != std::wstring::npos)
            {
                return label.substr(0, suffixPos);
            }
            return label;
        }

        void EnsureCurtainNumberBoxZero(NumberBox const& sender, bool& isLoading)
        {
            auto box = sender;
            if (std::isnan(box.Value()) || box.Text().empty())
            {
                isLoading = true;
                box.Value(0.0);
                isLoading = false;
            }
        }
    }

    MainWindow::MainWindow()
    {
        InitializeComponent();
        InitializeWindow();
        InitializeComboBoxes();
    }

    winrt::Windows::Foundation::IAsyncAction MainWindow::InitializeAsync()
    {
        LoadAllData();
        LoadHidSettingsUi();
        co_await winrt::resume_after(std::chrono::milliseconds(0));
    }

    void MainWindow::InitializeWindow()
    {
        VersionTextBlock().Text(BuildVersionText());
        m_csvService = std::make_unique<AppServices::CsvConfigService>(ResolveCsvPath());

        m_trayIcon.SetShowWindowCallback([this]() { ShowFromTray(); });
        m_trayIcon.SetExitCallback([this]() { RequestExit(); });

        Activated([this](IInspectable const&, IInspectable const&)
        {
            ApplyWindowIcon();
            if (!m_initialWindowSizeApplied)
            {
                CompletePlatformSetup();
            }
        });
    }

    void MainWindow::CompletePlatformSetup()
    {
        if (m_initialWindowSizeApplied)
        {
            return;
        }

        ApplyInitialWindowSize();
        SetupWindowCloseBehavior();
        StartHidMonitoring();
        UpdateTrayIconState();
        m_autoStart.SetEnabled(m_localSettings.LoadRunAtStartup());
        m_initialWindowSizeApplied = true;
    }

    void MainWindow::LaunchToTrayOnly()
    {
        Activate();
        CompletePlatformSetup();

        const HWND hwnd = GetWindowHandle();
        if (hwnd && !m_trayIcon.IsCreated())
        {
            m_trayIcon.Create(hwnd);
        }

        HideToTray();
    }

    void MainWindow::ApplyInitialWindowSize()
    {
        HWND hwnd = nullptr;
        if (auto windowNative = try_as<IWindowNative>())
        {
            windowNative->get_WindowHandle(&hwnd);
        }

        if (!hwnd)
        {
            return;
        }

        AppServices::WindowBoundsSpec bounds{
            kMinWindowWidth,
            kMinWindowHeight };
        m_windowBounds.Apply(hwnd, bounds);
        m_windowBounds.ResizeClientToLogicalSize(hwnd);
    }

    void MainWindow::ApplyWindowIcon()
    {
        HWND hwnd = nullptr;
        if (auto windowNative = try_as<IWindowNative>())
        {
            windowNative->get_WindowHandle(&hwnd);
        }

        AppServices::WindowIconHelper::Apply(hwnd);
    }

    void MainWindow::InitializeComboBoxes()
    {
        ClickModeComboBox().Items().Clear();
        ClickModeComboBox().Items().Append(box_value(L"与 Windows 系统设置保持一致"));
        ClickModeComboBox().Items().Append(box_value(L"自由调节"));

        TouchpadSensitivityComboBox().Items().Clear();
        TouchpadSensitivityComboBox().Items().Append(box_value(L"最高灵敏度 (0)"));
        TouchpadSensitivityComboBox().Items().Append(box_value(L"高灵敏度 (1)"));
        TouchpadSensitivityComboBox().Items().Append(box_value(L"中灵敏度 (2)"));
        TouchpadSensitivityComboBox().Items().Append(box_value(L"低灵敏度 (3)"));
    }

    std::filesystem::path MainWindow::ResolveCsvPath() const
    {
        wchar_t modulePath[MAX_PATH]{};
        GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
        auto path = std::filesystem::path(modulePath).parent_path() / L"config" / L"TouchpadPhysicalSize.csv";
        if (std::filesystem::exists(path))
        {
            return path;
        }
        return std::filesystem::path(L"config") / L"TouchpadPhysicalSize.csv";
    }

    void MainWindow::LoadAllData()
    {
        m_isLoading = true;

        m_registry.EnsureCurtainKeysExist();
        m_biosIdentity = m_bios.ReadIdentity();
        LaptopModelText().Text(m_biosIdentity.DisplayName());

        m_csvFileExists = std::filesystem::exists(ResolveCsvPath());
        m_matchedEntry = m_csvService->Match(m_biosIdentity);

        m_clickMode = m_localSettings.LoadClickSensitivityMode();
        ClickModeComboBox().SelectedIndex(m_clickMode == AppServices::ClickSensitivityMode::FreeAdjust ? 1 : 0);

        if (auto click = m_registry.APP_ClickForceSensitivity())
        {
            ClickSensitivitySlider().Value(static_cast<double>(*click));
            UpdateClickSensitivityLabel(static_cast<int>(*click));
        }
        else
        {
            ClickSensitivitySlider().Value(50);
            UpdateClickSensitivityLabel(50);
        }

        if (auto aap = m_registry.APP_AAPThreshold())
        {
            const int index = std::clamp(static_cast<int>(*aap), 0, 3);
            TouchpadSensitivityComboBox().SelectedIndex(index);
        }
        else
        {
            TouchpadSensitivityComboBox().SelectedIndex(2);
        }

        if (auto taps = m_registry.APP_TapsEnabled())
        {
            TapsEnabledSwitch().IsOn(*taps != 0);
        }
        else
        {
            TapsEnabledSwitch().IsOn(true);
        }

        const auto curtains = m_registry.APP_CurtainMm();
        const bool smartAreaEnabled = curtains.topMm > 0.0 || curtains.bottomMm > 0.0 || curtains.leftMm > 0.0 || curtains.rightMm > 0.0;
        SmartAreaSwitch().IsOn(smartAreaEnabled);
        SetCurtainUi(smartAreaEnabled ? curtains : AppServices::EdgeValues{}, smartAreaEnabled);

        const auto superCurtains = m_registry.APP_SuperCurtainMm();
        const bool smartEdgeEnabled = superCurtains.topMm > 0.0 || superCurtains.bottomMm > 0.0 || superCurtains.leftMm > 0.0 || superCurtains.rightMm > 0.0;
        SmartEdgeSwitch().IsOn(smartEdgeEnabled);
        SetSuperCurtainUi(smartEdgeEnabled ? superCurtains : AppServices::EdgeValues{}, smartEdgeEnabled);

        const auto savedSize = m_localSettings.LoadTouchpadSize();
        if (savedSize.hasValue)
        {
            TouchpadWidthBox().Value(savedSize.widthMm);
            TouchpadHeightBox().Value(savedSize.heightMm);
        }
        else
        {
            TouchpadWidthBox().Value(AppServices::kDefaultTouchpadWidthMm);
            TouchpadHeightBox().Value(AppServices::kDefaultTouchpadHeightMm);
        }

        UpdateMatchedSizeUi();
        UpdateExportButtonVisibility();
        RefreshDiagram();

        m_isLoading = false;
        m_isInitialized = true;
    }

    void MainWindow::RefreshButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        LoadAllData();
    }

    void MainWindow::ClickSensitivitySlider_ValueChanged(IInspectable const&, Controls::Primitives::RangeBaseValueChangedEventArgs const& args)
    {
        if (m_isLoading)
        {
            return;
        }

        int value = static_cast<int>(args.NewValue());
        value = SnapClickSensitivity(value);
        if (value != static_cast<int>(args.NewValue()))
        {
            m_isLoading = true;
            ClickSensitivitySlider().Value(static_cast<double>(value));
            m_isLoading = false;
        }

        UpdateClickSensitivityLabel(value);
        if (!m_registry.APP_SetClickForceSensitivity(static_cast<uint32_t>(value)))
        {
            Logger::Error(L"Failed to write ClickForceSensitivity=" + std::to_wstring(value));
        }
        else
        {
            Logger::Info(L"ClickForceSensitivity updated to " + std::to_wstring(value));
        }
    }

    void MainWindow::ClickModeComboBox_SelectionChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        if (m_isLoading || ClickModeComboBox().SelectedIndex() < 0)
        {
            return;
        }

        m_clickMode = ClickModeComboBox().SelectedIndex() == 1
            ? AppServices::ClickSensitivityMode::FreeAdjust
            : AppServices::ClickSensitivityMode::MatchWindowsSettings;
        m_localSettings.SaveClickSensitivityMode(m_clickMode);

        const int snapped = SnapClickSensitivity(static_cast<int>(ClickSensitivitySlider().Value()));
        m_isLoading = true;
        ClickSensitivitySlider().Value(static_cast<double>(snapped));
        m_isLoading = false;
        UpdateClickSensitivityLabel(snapped);
        if (!m_registry.APP_SetClickForceSensitivity(static_cast<uint32_t>(snapped)))
        {
            Logger::Error(L"Failed to write ClickForceSensitivity=" + std::to_wstring(snapped));
        }
    }

    void MainWindow::TouchpadSensitivityComboBox_SelectionChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        if (m_isLoading || TouchpadSensitivityComboBox().SelectedIndex() < 0)
        {
            return;
        }

        const uint32_t value = static_cast<uint32_t>(TouchpadSensitivityComboBox().SelectedIndex());
        if (!m_registry.APP_SetAAPThreshold(value))
        {
            Logger::Error(L"Failed to write AAPThreshold=" + std::to_wstring(value));
        }
    }

    void MainWindow::TapsEnabledSwitch_Toggled(IInspectable const&, RoutedEventArgs const&)
    {
        if (m_isLoading)
        {
            return;
        }

        m_registry.APP_SetTapsEnabled(TapsEnabledSwitch().IsOn() ? 1u : 0u);
    }

    void MainWindow::SmartAreaSwitch_Toggled(IInspectable const&, RoutedEventArgs const&)
    {
        if (m_isLoading)
        {
            return;
        }

        if (SmartAreaSwitch().IsOn())
        {
            SetCurtainUi(m_registry.APP_CurtainMm(), true);
        }
        else
        {
            AppServices::EdgeValues zero{};
            SetCurtainUi(zero, false);
            if (m_registry.APP_SetCurtainMm(zero))
            {
                MarkCurtainRestartRequired();
            }
        }
        RefreshDiagram();
    }

    void MainWindow::SmartEdgeSwitch_Toggled(IInspectable const&, RoutedEventArgs const&)
    {
        if (m_isLoading)
        {
            return;
        }

        if (SmartEdgeSwitch().IsOn())
        {
            SetSuperCurtainUi(m_registry.APP_SuperCurtainMm(), true);
        }
        else
        {
            AppServices::EdgeValues zero{};
            SetSuperCurtainUi(zero, false);
            if (m_registry.APP_SetSuperCurtainMm(zero))
            {
                MarkCurtainRestartRequired();
            }
        }
        RefreshDiagram();
    }

    void MainWindow::CurtainValueChanged(NumberBox const& sender, NumberBoxValueChangedEventArgs const&)
    {
        if (m_isLoading)
        {
            return;
        }

        EnsureCurtainNumberBoxZero(sender, m_isLoading);

        if (!SmartAreaSwitch().IsOn())
        {
            return;
        }

        if (m_registry.APP_SetCurtainMm(ReadCurtainFromUi()))
        {
            MarkCurtainRestartRequired();
        }
        RefreshDiagram();
    }

    void MainWindow::SuperCurtainValueChanged(NumberBox const& sender, NumberBoxValueChangedEventArgs const&)
    {
        if (m_isLoading)
        {
            return;
        }

        EnsureCurtainNumberBoxZero(sender, m_isLoading);

        if (!SmartEdgeSwitch().IsOn())
        {
            return;
        }

        if (m_registry.APP_SetSuperCurtainMm(ReadSuperCurtainFromUi()))
        {
            MarkCurtainRestartRequired();
        }
        RefreshDiagram();
    }

    void MainWindow::TouchpadSizeValueChanged(NumberBox const&, NumberBoxValueChangedEventArgs const&)
    {
        if (m_isLoading || !m_isInitialized)
        {
            return;
        }

        const double width = TouchpadWidthBox().Value();
        const double height = TouchpadHeightBox().Value();
        if (width > 0.0 && height > 0.0)
        {
            m_localSettings.SaveTouchpadSize(width, height);
            UpdateMatchedSizeUi();
            UpdateExportButtonVisibility();
            RefreshDiagram();
        }
    }

    void MainWindow::ApplyMatchedSizeButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        if (!m_matchedEntry.has_value())
        {
            return;
        }

        m_isLoading = true;
        TouchpadWidthBox().Value(m_matchedEntry->widthMm);
        TouchpadHeightBox().Value(m_matchedEntry->heightMm);
        m_isLoading = false;

        m_localSettings.SaveTouchpadSize(m_matchedEntry->widthMm, m_matchedEntry->heightMm);
        UpdateMatchedSizeUi();
        UpdateExportButtonVisibility();
        RefreshDiagram();
    }

    winrt::Windows::Foundation::IAsyncAction MainWindow::ExportTouchpadSizeButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        const double width = TouchpadWidthBox().Value();
        const double height = TouchpadHeightBox().Value();
        if (width <= 0.0 || height <= 0.0)
        {
            Logger::Error(L"Touchpad size must be greater than zero before export");
            co_return;
        }

        AppServices::TouchpadPhysicalSizeEntry entry{};
        entry.identity = m_biosIdentity;
        entry.widthMm = width;
        entry.heightMm = height;

        if (!m_csvService->UpsertEntry(entry))
        {
            Logger::Error(L"Failed to export touchpad physical size to TouchpadPhysicalSize.csv");
            co_return;
        }

        m_matchedEntry = entry;
        m_csvFileExists = true;
        UpdateMatchedSizeUi();
        UpdateExportButtonVisibility();

        ContentDialog dialog{};
        dialog.XamlRoot(Content().XamlRoot());
        dialog.Title(box_value(L"导出成功"));
        dialog.Content(box_value(
            L"已将当前触控板物理尺寸设定写入 TouchpadPhysicalSize.csv。\n\n"
            + ResolveCsvPath().wstring()));
        dialog.CloseButtonText(L"确定");
        dialog.DefaultButton(ContentDialogButton::Close);
        co_await dialog.ShowAsync();
    }

    void MainWindow::DiagramCanvas_SizeChanged(IInspectable const&, SizeChangedEventArgs const& args)
    {
        if (m_isLoading || args.NewSize().Width <= 0.0)
        {
            return;
        }

        RefreshDiagram();
    }

    void MainWindow::MoreTouchpadSettings_Click(IInspectable const&, RoutedEventArgs const&)
    {
        Windows::System::Launcher::LaunchUriAsync(Windows::Foundation::Uri(L"ms-settings:devices-touchpad"));
    }

    winrt::Windows::Foundation::IAsyncAction MainWindow::RestartButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        ContentDialog dialog{};
        dialog.XamlRoot(Content().XamlRoot());
        dialog.Title(box_value(L"重启电脑"));
        dialog.Content(box_value(
            L"电脑重启后，您未保存的修订都将丢失，请先保存所有需要保存的文档，然后再重启电脑。"));
        dialog.PrimaryButtonText(L"取消重启");
        dialog.SecondaryButtonText(L"立即重启");
        dialog.DefaultButton(ContentDialogButton::Primary);

        const auto result = co_await dialog.ShowAsync();
        if (result == ContentDialogResult::Secondary)
        {
            PerformSystemReboot();
        }
    }

    void MainWindow::PerformSystemReboot()
    {
        HANDLE token = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
        {
            Logger::Error(L"OpenProcessToken failed for restart: " + std::to_wstring(GetLastError()));
            return;
        }

        TOKEN_PRIVILEGES privileges{};
        privileges.PrivilegeCount = 1;
        privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        if (!LookupPrivilegeValueW(nullptr, SE_SHUTDOWN_NAME, &privileges.Privileges[0].Luid))
        {
            Logger::Error(L"LookupPrivilegeValue failed for restart: " + std::to_wstring(GetLastError()));
            CloseHandle(token);
            return;
        }

        AdjustTokenPrivileges(token, FALSE, &privileges, 0, nullptr, nullptr);
        CloseHandle(token);

        if (!ExitWindowsEx(EWX_REBOOT, SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_MAINTENANCE))
        {
            Logger::Error(L"ExitWindowsEx reboot failed: " + std::to_wstring(GetLastError()));
        }
    }

    void MainWindow::MarkCurtainRestartRequired()
    {
        m_curtainRestartPending = true;
        UpdateRestartButtonVisibility();
    }

    void MainWindow::UpdateRestartButtonVisibility()
    {
        RestartPanel().Visibility(m_curtainRestartPending ? Visibility::Visible : Visibility::Collapsed);
    }

    void MainWindow::RefreshDiagram()
    {
        AppServices::DiagramRenderInput input{};
        input.touchpadWidthMm = TouchpadWidthBox().Value();
        input.touchpadHeightMm = TouchpadHeightBox().Value();
        input.curtain = SmartAreaSwitch().IsOn() ? ReadCurtainFromUi() : AppServices::EdgeValues{};
        input.superCurtain = SmartEdgeSwitch().IsOn() ? ReadSuperCurtainFromUi() : AppServices::EdgeValues{};

        HWND hwnd = nullptr;
        if (auto windowNative = try_as<IWindowNative>())
        {
            windowNative->get_WindowHandle(&hwnd);
        }
        const auto displayMetrics = m_displayScale.GetDisplayMetricsForWindow(hwnd);
        input.mmPerDip = m_displayScale.MillimetersPerPixel(displayMetrics);

        const auto result = m_diagramRenderer.Render(DiagramCanvas(), input);
        if (result.overlapEdgeLabels.empty())
        {
            DiagramWarningText().Text(L"");
        }
        else
        {
            std::wstring edgeList;
            for (auto const& edgeLabel : result.overlapEdgeLabels)
            {
                if (!edgeList.empty())
                {
                    edgeList += L"、";
                }
                edgeList += edgeLabel;
            }

            const std::wstring summary =
                L"以下触控板区域的【防误触区域】≥ 【缓冲区域】，这些区域将完全以【防误触区域】的处理逻辑进行控制：";
            DiagramWarningText().Text(summary + L"\n" + edgeList);
        }
    }

    void MainWindow::UpdateClickSensitivityLabel(int value)
    {
        ClickSensitivityValueText().Text(ClickSensitivityLabel(value) + L"  " + std::to_wstring(value));
    }

    int MainWindow::SnapClickSensitivity(int value) const
    {
        if (m_clickMode == AppServices::ClickSensitivityMode::FreeAdjust)
        {
            return std::clamp(value, 0, 100);
        }

        const int candidates[] = { 0, 50, 100 };
        int best = candidates[0];
        int bestDistance = abs(value - best);
        for (int candidate : candidates)
        {
            const int distance = abs(value - candidate);
            if (distance < bestDistance)
            {
                best = candidate;
                bestDistance = distance;
            }
        }
        return best;
    }

    std::wstring MainWindow::ClickSensitivityLabel(int value) const
    {
        switch (value)
        {
        case 0: return L"轻";
        case 50: return L"中";
        case 100: return L"重";
        default: return L"";
        }
    }

    AppServices::EdgeValues MainWindow::ReadCurtainFromUi()
    {
        return {
            NumberBoxValueOrZero(CurtainTopBox()),
            NumberBoxValueOrZero(CurtainBottomBox()),
            NumberBoxValueOrZero(CurtainLeftBox()),
            NumberBoxValueOrZero(CurtainRightBox())
        };
    }

    AppServices::EdgeValues MainWindow::ReadSuperCurtainFromUi()
    {
        return {
            NumberBoxValueOrZero(SuperCurtainTopBox()),
            NumberBoxValueOrZero(SuperCurtainBottomBox()),
            NumberBoxValueOrZero(SuperCurtainLeftBox()),
            NumberBoxValueOrZero(SuperCurtainRightBox())
        };
    }

    void MainWindow::SetCurtainUi(AppServices::EdgeValues const& values, bool enabled)
    {
        m_isLoading = true;
        CurtainTopBox().Value(values.topMm);
        CurtainBottomBox().Value(values.bottomMm);
        CurtainLeftBox().Value(values.leftMm);
        CurtainRightBox().Value(values.rightMm);
        CurtainTopBox().IsEnabled(enabled);
        CurtainBottomBox().IsEnabled(enabled);
        CurtainLeftBox().IsEnabled(enabled);
        CurtainRightBox().IsEnabled(enabled);
        m_isLoading = false;
    }

    void MainWindow::SetSuperCurtainUi(AppServices::EdgeValues const& values, bool enabled)
    {
        m_isLoading = true;
        SuperCurtainTopBox().Value(values.topMm);
        SuperCurtainBottomBox().Value(values.bottomMm);
        SuperCurtainLeftBox().Value(values.leftMm);
        SuperCurtainRightBox().Value(values.rightMm);
        SuperCurtainTopBox().IsEnabled(enabled);
        SuperCurtainBottomBox().IsEnabled(enabled);
        SuperCurtainLeftBox().IsEnabled(enabled);
        SuperCurtainRightBox().IsEnabled(enabled);
        m_isLoading = false;
    }

    void MainWindow::UpdateMatchedSizeUi()
    {
        if (!m_matchedEntry.has_value())
        {
            MatchedSizeText().Visibility(Visibility::Collapsed);
            ApplyMatchedSizeButton().Visibility(Visibility::Collapsed);
            return;
        }

        const double currentWidth = TouchpadWidthBox().Value();
        const double currentHeight = TouchpadHeightBox().Value();
        const bool sameSize = AppServices::SameTouchpadSizeMm(
            currentWidth,
            currentHeight,
            m_matchedEntry->widthMm,
            m_matchedEntry->heightMm);

        if (sameSize)
        {
            MatchedSizeText().Visibility(Visibility::Collapsed);
            ApplyMatchedSizeButton().Visibility(Visibility::Collapsed);
            return;
        }

        MatchedSizeText().Visibility(Visibility::Visible);
        ApplyMatchedSizeButton().Visibility(Visibility::Visible);
        MatchedSizeText().Text(
            L"配置文件中匹配到的触控板物理尺寸："
            + AppServices::FormatMm(m_matchedEntry->widthMm)
            + L" mm × "
            + AppServices::FormatMm(m_matchedEntry->heightMm)
            + L" mm");
    }

    void MainWindow::UpdateExportButtonVisibility()
    {
        const double currentWidth = TouchpadWidthBox().Value();
        const double currentHeight = TouchpadHeightBox().Value();
        if (currentWidth <= 0.0 || currentHeight <= 0.0)
        {
            ExportTouchpadSizeButton().Visibility(Visibility::Collapsed);
            return;
        }

        const bool csvExists = m_csvFileExists;
        bool shouldShow = false;

        if (!csvExists)
        {
            shouldShow = true;
        }
        else if (!m_matchedEntry.has_value())
        {
            shouldShow = true;
        }
        else
        {
            shouldShow = !AppServices::SameTouchpadSizeMm(
                currentWidth,
                currentHeight,
                m_matchedEntry->widthMm,
                m_matchedEntry->heightMm);
        }

        ExportTouchpadSizeButton().Visibility(shouldShow ? Visibility::Visible : Visibility::Collapsed);
    }

    HWND MainWindow::GetWindowHandle() const
    {
        HWND hwnd = nullptr;
        if (auto windowNative = try_as<IWindowNative>())
        {
            windowNative->get_WindowHandle(&hwnd);
        }
        return hwnd;
    }

    void MainWindow::SetupWindowCloseBehavior()
    {
        const HWND hwnd = GetWindowHandle();
        if (!hwnd)
        {
            return;
        }

        const auto windowId = Microsoft::UI::GetWindowIdFromWindow(hwnd);
        if (auto appWindow = AppWindow::GetFromWindowId(windowId))
        {
            appWindow.Closing([this](IInspectable const&, AppWindowClosingEventArgs const& args)
            {
                if (m_forceExit)
                {
                    return;
                }

                if (ShouldMinimizeToTrayOnClose())
                {
                    args.Cancel(true);
                    HideToTray();
                }
            });
        }
    }

    bool MainWindow::ShouldMinimizeToTrayOnClose() const
    {
        return m_localSettings.LoadMinimizeToTrayOnClose() || m_localSettings.LoadHidAutoTouchpadEnabled();
    }

    void MainWindow::HideToTray()
    {
        UpdateTrayIconState();
        if (const HWND hwnd = GetWindowHandle())
        {
            ShowWindow(hwnd, SW_HIDE);
        }
    }

    void MainWindow::ShowFromTray()
    {
        Activate();
        if (const HWND hwnd = GetWindowHandle())
        {
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);
        }
    }

    void MainWindow::RequestExit()
    {
        m_forceExit = true;
        StopHidMonitoring();
        m_trayIcon.Destroy();
        Close();
        Application::Current().Exit();
    }

    void MainWindow::UpdateTrayIconState()
    {
        const bool shouldCreate =
            m_localSettings.LoadMinimizeToTrayOnClose() || m_localSettings.LoadHidAutoTouchpadEnabled();
        const HWND hwnd = GetWindowHandle();
        if (!hwnd)
        {
            return;
        }

        if (shouldCreate)
        {
            if (!m_trayIcon.IsCreated())
            {
                m_trayIcon.Create(hwnd);
            }
        }
        else
        {
            m_trayIcon.Destroy();
        }
    }

    void MainWindow::ApplyHidPolicyLocks()
    {
        const bool hidEnabled = HidAutoTouchpadSwitch().IsOn();
        RunAtStartupSwitch().IsEnabled(!hidEnabled);
        MinimizeToTraySwitch().IsEnabled(!hidEnabled);

        if (hidEnabled)
        {
            HidPolicyHintText().Text(
                L"已启用 HID 自动启停：为保证插拔监听持续有效，开机自启动与常驻系统托盘已强制开启。");
        }
        else
        {
            HidPolicyHintText().Text(
                L"启用 HID 自动启停后，将强制开启开机自启动与常驻系统托盘，以确保后台监听。");
        }
    }

    void MainWindow::LoadHidSettingsUi()
    {
        m_hidSettingsLoading = true;

        m_monitoredHidDevices = m_localSettings.LoadMonitoredHidDevices();
        for (auto& device : m_monitoredHidDevices)
        {
            device.label = StripVidPidSuffix(std::move(device.label));
        }
        HidAutoTouchpadSwitch().IsOn(m_localSettings.LoadHidAutoTouchpadEnabled());
        RunAtStartupSwitch().IsOn(m_localSettings.LoadRunAtStartup());
        MinimizeToTraySwitch().IsOn(m_localSettings.LoadMinimizeToTrayOnClose());

        if (HidAutoTouchpadSwitch().IsOn())
        {
            if (!RunAtStartupSwitch().IsOn())
            {
                RunAtStartupSwitch().IsOn(true);
                m_localSettings.SaveRunAtStartup(true);
                m_autoStart.SetEnabled(true);
            }
            if (!MinimizeToTraySwitch().IsOn())
            {
                MinimizeToTraySwitch().IsOn(true);
                m_localSettings.SaveMinimizeToTrayOnClose(true);
            }
        }

        ApplyHidPolicyLocks();
        RefreshAvailableHidDevices();

        m_hidSettingsLoading = false;
    }

    void MainWindow::RefreshAvailableHidDevices()
    {
        m_availableHidDevices = m_hidEnumeration.ListConnectedHidDevices(true);

        bool labelsChanged = false;
        for (auto& monitored : m_monitoredHidDevices)
        {
            for (auto const& online : m_availableHidDevices)
            {
                if (AppServices::HidDevicesMatch(monitored, online) && monitored.label != online.label)
                {
                    monitored.label = online.label;
                    labelsChanged = true;
                }
            }
        }

        if (labelsChanged)
        {
            SaveMonitoredHidDevices();
        }

        RefreshHidDeviceListsUi();
    }

    void MainWindow::RefreshHidDeviceListsUi()
    {
        MonitoredHidListView().Items().Clear();
        UnmonitoredHidListView().Items().Clear();

        for (size_t i = 0; i < m_monitoredHidDevices.size(); ++i)
        {
            Grid row{};
            row.ColumnSpacing(8);
            row.Padding({ kHidDeviceRowIndent, 4, 0, 4 });

            ColumnDefinition labelColumn{};
            ColumnDefinition buttonColumn{};
            buttonColumn.Width(GridLengthHelper::Auto());
            row.ColumnDefinitions().Append(labelColumn);
            row.ColumnDefinitions().Append(buttonColumn);

            TextBlock label{};
            label.Text(m_monitoredHidDevices[i].label);
            label.TextWrapping(TextWrapping::WrapWholeWords);
            label.VerticalAlignment(VerticalAlignment::Center);
            if (!IsMonitoredDeviceOnline(m_monitoredHidDevices[i], m_availableHidDevices))
            {
                label.Opacity(kOfflineHidDeviceLabelOpacity);
            }
            Grid::SetColumn(label, 0);

            Button removeButton{};
            removeButton.Content(box_value(L"移除"));
            removeButton.VerticalAlignment(VerticalAlignment::Center);
            Grid::SetColumn(removeButton, 1);
            removeButton.Click([this, i](IInspectable const&, RoutedEventArgs const&)
            {
                RemoveMonitoredHidDevice(i);
            });

            row.Children().Append(label);
            row.Children().Append(removeButton);
            MonitoredHidListView().Items().Append(row);
        }

        for (size_t i = 0; i < m_availableHidDevices.size(); ++i)
        {
            const auto& device = m_availableHidDevices[i];
            const bool alreadyMonitored = std::any_of(
                m_monitoredHidDevices.begin(),
                m_monitoredHidDevices.end(),
                [&](AppServices::MonitoredHidDevice const& item)
                {
                    return AppServices::HidDevicesMatch(item, device);
                });
            if (alreadyMonitored)
            {
                continue;
            }

            Grid row{};
            row.ColumnSpacing(8);
            row.Padding({ kHidDeviceRowIndent, 4, 0, 4 });

            ColumnDefinition labelColumn{};
            ColumnDefinition buttonColumn{};
            buttonColumn.Width(GridLengthHelper::Auto());
            row.ColumnDefinitions().Append(labelColumn);
            row.ColumnDefinitions().Append(buttonColumn);

            TextBlock label{};
            label.Text(device.label);
            label.TextWrapping(TextWrapping::WrapWholeWords);
            label.VerticalAlignment(VerticalAlignment::Center);
            label.Opacity(kOfflineHidDeviceLabelOpacity);
            Grid::SetColumn(label, 0);

            Button addButton{};
            addButton.Content(box_value(L"添加"));
            addButton.VerticalAlignment(VerticalAlignment::Center);
            Grid::SetColumn(addButton, 1);
            addButton.Click([this, i](IInspectable const&, RoutedEventArgs const&)
            {
                if (i >= m_availableHidDevices.size())
                {
                    return;
                }

                AddMonitoredHidDevice(m_availableHidDevices[i]);
            });

            row.Children().Append(label);
            row.Children().Append(addButton);
            UnmonitoredHidListView().Items().Append(row);
        }
    }

    void MainWindow::AddMonitoredHidDevice(AppServices::HidDeviceInfo const& device)
    {
        const bool exists = std::any_of(
            m_monitoredHidDevices.begin(),
            m_monitoredHidDevices.end(),
            [&](AppServices::MonitoredHidDevice const& item)
            {
                return AppServices::HidDevicesMatch(item, device);
            });
        if (exists)
        {
            return;
        }

        AppServices::MonitoredHidDevice entry{};
        entry.vid = device.vid;
        entry.pid = device.pid;
        entry.label = device.label;
        m_monitoredHidDevices.push_back(std::move(entry));

        SaveMonitoredHidDevices();
        RefreshHidDeviceListsUi();
        if (m_localSettings.LoadHidAutoTouchpadEnabled())
        {
            m_hidMonitor.ReconcileNow();
        }
    }

    void MainWindow::RemoveMonitoredHidDevice(size_t index)
    {
        if (index >= m_monitoredHidDevices.size())
        {
            return;
        }

        m_monitoredHidDevices.erase(m_monitoredHidDevices.begin() + static_cast<std::ptrdiff_t>(index));
        SaveMonitoredHidDevices();
        RefreshHidDeviceListsUi();
        if (m_localSettings.LoadHidAutoTouchpadEnabled())
        {
            m_hidMonitor.ReconcileNow();
        }
    }

    void MainWindow::SaveMonitoredHidDevices()
    {
        m_localSettings.SaveMonitoredHidDevices(m_monitoredHidDevices);
        m_hidMonitor.SetMonitoredDevices(m_monitoredHidDevices);
    }

    void MainWindow::StartHidMonitoring()
    {
        const bool enabled = m_localSettings.LoadHidAutoTouchpadEnabled();
        m_hidMonitor.SetEnabled(enabled);
        m_hidMonitor.SetMonitoredDevices(m_monitoredHidDevices);
        m_hidMonitor.SetDeviceChangeCallback([this]()
        {
            RefreshAvailableHidDevices();
        });

        const HWND hwnd = GetWindowHandle();
        if (!hwnd)
        {
            return;
        }

        m_hidMonitor.RegisterNotifications(hwnd);

        if (enabled)
        {
            m_hidMonitor.ReconcileNow();
        }
    }

    void MainWindow::StopHidMonitoring()
    {
        m_hidMonitor.SetEnabled(false);
        m_hidMonitor.UnregisterNotifications();
    }

    void MainWindow::HidAutoTouchpadSwitch_Toggled(IInspectable const&, RoutedEventArgs const&)
    {
        if (m_hidSettingsLoading)
        {
            return;
        }

        const bool enabled = HidAutoTouchpadSwitch().IsOn();
        m_localSettings.SaveHidAutoTouchpadEnabled(enabled);

        if (enabled)
        {
            m_hidSettingsLoading = true;
            RunAtStartupSwitch().IsOn(true);
            MinimizeToTraySwitch().IsOn(true);
            m_hidSettingsLoading = false;

            m_localSettings.SaveRunAtStartup(true);
            m_localSettings.SaveMinimizeToTrayOnClose(true);
            m_autoStart.SetEnabled(true);
        }

        ApplyHidPolicyLocks();
        UpdateTrayIconState();
        StartHidMonitoring();
    }

    void MainWindow::HidRefreshButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        RefreshAvailableHidDevices();
    }

    void MainWindow::RunAtStartupSwitch_Toggled(IInspectable const&, RoutedEventArgs const&)
    {
        if (m_hidSettingsLoading || HidAutoTouchpadSwitch().IsOn())
        {
            return;
        }

        const bool enabled = RunAtStartupSwitch().IsOn();
        m_localSettings.SaveRunAtStartup(enabled);
        m_autoStart.SetEnabled(enabled);
    }

    void MainWindow::MinimizeToTraySwitch_Toggled(IInspectable const&, RoutedEventArgs const&)
    {
        if (m_hidSettingsLoading || HidAutoTouchpadSwitch().IsOn())
        {
            return;
        }

        const bool enabled = MinimizeToTraySwitch().IsOn();
        m_localSettings.SaveMinimizeToTrayOnClose(enabled);
        UpdateTrayIconState();
    }
}

#include "Generated Files/MainWindow.xaml.g.hpp"
