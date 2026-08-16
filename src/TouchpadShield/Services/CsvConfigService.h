#pragma once

#include "Services/BiosService.h"

#include <filesystem>
#include <optional>

namespace TouchpadShield::Services
{
    struct TouchpadPhysicalSizeEntry
    {
        BiosIdentity identity;
        double widthMm{ 0.0 };
        double heightMm{ 0.0 };
    };

    class CsvConfigService
    {
    public:
        explicit CsvConfigService(std::filesystem::path csvPath);

        std::optional<TouchpadPhysicalSizeEntry> Match(BiosIdentity const& identity) const;
        bool UpsertEntry(TouchpadPhysicalSizeEntry const& entry);

    private:
        std::filesystem::path m_csvPath;

        static constexpr wchar_t kHeaderLine[] =
            L"SystemManufacturer,SystemProductName,SystemSKU,SystemVersion,TouchpadWidth,TouchpadHeight";

        static bool IdentityEquals(BiosIdentity const& left, BiosIdentity const& right);
        static std::wstring EscapeCsvField(std::wstring const& value);
        static std::wstring FormatCsvRow(TouchpadPhysicalSizeEntry const& entry);
        static std::vector<std::wstring> SplitCsvLine(std::wstring const& line);
    };
}
