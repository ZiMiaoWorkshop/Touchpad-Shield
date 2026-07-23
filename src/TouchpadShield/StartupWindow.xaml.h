#pragma once

#include "StartupWindow.g.h"
#include "StartupWindow.xaml.g.h"

namespace winrt::TouchpadShield::implementation
{
    struct StartupWindow : StartupWindowT<StartupWindow>
    {
        StartupWindow();
    };
}

namespace winrt::TouchpadShield::factory_implementation
{
    struct StartupWindow : StartupWindowT<StartupWindow, implementation::StartupWindow>
    {
    };
}
