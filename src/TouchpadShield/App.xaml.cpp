#include "pch.h"
#include "App.xaml.h"
#include "StartupWindow.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::TouchpadShield::implementation
{
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
        auto startupWindow = make<StartupWindow>();
        startupWindow.Activate();
        co_await winrt::resume_after(std::chrono::milliseconds(0));

        m_window = make<MainWindow>();
        co_await winrt::get_self<implementation::MainWindow>(m_window)->InitializeAsync();

        startupWindow.Close();
        m_window.Activate();
    }
}

#include "Generated Files/App.xaml.g.hpp"
