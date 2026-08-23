#include "pch.h"
#include "App.xaml.h"
#include "MainWindow.xaml.h"
#include "StartupWindow.xaml.h"
#include "Services/AutoStartService.h"
#include "Services/SingleInstanceService.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::TouchpadShield::implementation
{
    namespace
    {
        ::TouchpadShield::Services::SingleInstanceService g_singleInstance{};
    }

    App::App()
    {
        Logger::Info(L"Touchpad Shield starting");
    }

    void App::OnLaunched(LaunchActivatedEventArgs const&)
    {
        LaunchAsync();
    }

    Windows::Foundation::IAsyncAction App::LaunchAsync()
    {
        const bool startupLaunch = ::TouchpadShield::Services::AutoStartService::IsStartupLaunch();

        if (!g_singleInstance.TryAcquire())
        {
            if (!startupLaunch)
            {
                g_singleInstance.ActivateExistingInstance();
            }

            Application::Current().Exit();
            co_return;
        }

        winrt::TouchpadShield::StartupWindow startupWindow{ nullptr };
        if (!startupLaunch)
        {
            startupWindow = make<StartupWindow>();
            startupWindow.Activate();
            co_await winrt::resume_after(std::chrono::milliseconds(0));
        }

        m_window = make<MainWindow>();

        auto* mainWindow = winrt::get_self<implementation::MainWindow>(m_window);
        if (startupLaunch)
        {
            mainWindow->PrepareSilentStartup();
        }

        m_window.Closed([](IInspectable const&, WindowEventArgs const&)
        {
            g_singleInstance.Release();
        });

        co_await mainWindow->InitializeAsync();

        if (startupWindow)
        {
            startupWindow.Close();
        }

        if (startupLaunch)
        {
            Logger::Info(L"Startup launch: hiding main window to tray");
            winrt::get_self<implementation::MainWindow>(m_window)->LaunchToTrayOnly();
        }
        else
        {
            m_window.Activate();
        }
    }
}

#include "Generated Files/App.xaml.g.hpp"
