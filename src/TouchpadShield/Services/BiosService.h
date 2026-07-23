#pragma once

#include <optional>
#include <string>

namespace TouchpadShield::Services
{
    struct BiosIdentity
    {
        std::wstring manufacturer;
        std::wstring productName;
        std::wstring sku;
        std::wstring version;

        std::wstring DisplayName() const;
    };

    class BiosService
    {
    public:
        BiosIdentity ReadIdentity() const;

    private:
        std::wstring ReadString(HKEY root, std::wstring const& subKey, std::wstring const& valueName) const;
    };
}
