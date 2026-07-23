#include "pch.h"
#include "Services/BiosService.h"

namespace TouchpadShield::Services
{
    std::wstring BiosIdentity::DisplayName() const
    {
        std::wstring result = manufacturer;
        auto appendPart = [&result](std::wstring const& part)
        {
            if (!part.empty())
            {
                result += L" - " + part;
            }
        };

        appendPart(productName);
        appendPart(sku);
        appendPart(version);
        return result;
    }

    BiosIdentity BiosService::ReadIdentity() const
    {
        static constexpr wchar_t kPath[] = L"HARDWARE\\DESCRIPTION\\System\\BIOS";
        return {
            ReadString(HKEY_LOCAL_MACHINE, kPath, L"SystemManufacturer"),
            ReadString(HKEY_LOCAL_MACHINE, kPath, L"SystemProductName"),
            ReadString(HKEY_LOCAL_MACHINE, kPath, L"SystemSKU"),
            ReadString(HKEY_LOCAL_MACHINE, kPath, L"SystemVersion")
        };
    }

    std::wstring BiosService::ReadString(HKEY root, std::wstring const& subKey, std::wstring const& valueName) const
    {
        HKEY key = nullptr;
        if (RegOpenKeyExW(root, subKey.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS)
        {
            return L"";
        }

        DWORD type = REG_SZ;
        DWORD size = 0;
        if (RegQueryValueExW(key, valueName.c_str(), nullptr, &type, nullptr, &size) != ERROR_SUCCESS || size == 0)
        {
            RegCloseKey(key);
            return L"";
        }

        std::vector<wchar_t> buffer(size / sizeof(wchar_t) + 1, L'\0');
        if (RegQueryValueExW(key, valueName.c_str(), nullptr, &type, reinterpret_cast<LPBYTE>(buffer.data()), &size) != ERROR_SUCCESS)
        {
            RegCloseKey(key);
            return L"";
        }

        RegCloseKey(key);
        return std::wstring(buffer.data());
    }
}
