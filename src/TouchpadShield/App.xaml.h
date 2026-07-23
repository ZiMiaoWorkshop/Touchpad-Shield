#pragma once

#include "App.g.h"
#include "App.xaml.g.h"

namespace winrt::TouchpadShield::implementation
{
    struct App : AppT<App>
    {
        App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

    private:
        winrt::Windows::Foundation::IAsyncAction LaunchAsync();
        winrt::TouchpadShield::MainWindow m_window{ nullptr };
    };
}

namespace winrt::TouchpadShield::factory_implementation
{
    struct App : AppT<App, implementation::App>
    {
    };
}
