#include "pch.h"
#include "StartupWindow.xaml.h"
#include "Services/WindowIconHelper.h"

#include <microsoft.ui.xaml.window.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::TouchpadShield::implementation
{
    StartupWindow::StartupWindow()
    {
        InitializeComponent();

        Activated([this](IInspectable const&, IInspectable const&)
        {
            HWND hwnd = nullptr;
            if (auto windowNative = try_as<IWindowNative>())
            {
                windowNative->get_WindowHandle(&hwnd);
            }

            ::TouchpadShield::Services::WindowIconHelper::Apply(hwnd);
        });
    }
}

#include "Generated Files/StartupWindow.xaml.g.hpp"
