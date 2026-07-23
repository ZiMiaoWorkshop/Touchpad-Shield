#pragma once

struct HWND__;

namespace TouchpadShield::Services
{
    class WindowIconHelper
    {
    public:
        static void Apply(HWND__* hwnd);
    };
}
