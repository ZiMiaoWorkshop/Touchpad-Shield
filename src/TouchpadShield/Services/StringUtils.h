#pragma once

#include <algorithm>
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

    inline std::wstring ToUpper(std::wstring value)
    {
        std::transform(value.begin(), value.end(), value.begin(), ::towupper);
        return value;
    }

    inline std::wstring ToLower(std::wstring value)
    {
        std::transform(value.begin(), value.end(), value.begin(), ::towlower);
        return value;
    }

    inline bool ContainsInsensitive(std::wstring const& haystack, std::wstring const& needle)
    {
        if (needle.empty())
        {
            return false;
        }

        const std::wstring lowerHay = ToLower(haystack);
        const std::wstring lowerNeedle = ToLower(needle);
        return lowerHay.find(lowerNeedle) != std::wstring::npos;
    }
}
