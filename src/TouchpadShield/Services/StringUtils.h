#pragma once

#include <cwctype>
#include <string>

namespace TouchpadShield::Services
{
    inline std::wstring Trim(std::wstring value)
    {
        while (!value.empty() && iswspace(value.front()))
        {
            value.erase(value.begin());
        }
        while (!value.empty() && iswspace(value.back()))
        {
            value.pop_back();
        }
        return value;
    }
}
