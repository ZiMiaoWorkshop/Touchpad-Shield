#include "pch.h"
#include "Services/CsvConfigService.h"
#include "Services/StringUtils.h"
#include "Services/UnitConversion.h"

namespace TouchpadShield::Services
{
    namespace
    {
        std::optional<TouchpadPhysicalSizeEntry> ParseEntryRow(std::vector<std::wstring> const& columns)
        {
            if (columns.size() < 6)
            {
                return std::nullopt;
            }

            TouchpadPhysicalSizeEntry entry{};
            entry.identity = {
                columns[0], columns[1], columns[2], columns[3]
            };
            entry.widthMm = wcstod(columns[4].c_str(), nullptr);
            entry.heightMm = wcstod(columns[5].c_str(), nullptr);
            return entry;
        }
    }

    CsvConfigService::CsvConfigService(std::filesystem::path csvPath)
        : m_csvPath(std::move(csvPath))
    {
    }

    bool CsvConfigService::IdentityEquals(BiosIdentity const& left, BiosIdentity const& right)
    {
        return left.manufacturer == right.manufacturer &&
            left.productName == right.productName &&
            left.sku == right.sku &&
            left.version == right.version;
    }

    std::wstring CsvConfigService::EscapeCsvField(std::wstring const& value)
    {
        if (value.find(L',') == std::wstring::npos && value.find(L'"') == std::wstring::npos)
        {
            return value;
        }

        std::wstring escaped;
        escaped.reserve(value.size() + 2);
        escaped.push_back(L'"');
        for (wchar_t ch : value)
        {
            if (ch == L'"')
            {
                escaped.push_back(L'"');
            }
            escaped.push_back(ch);
        }
        escaped.push_back(L'"');
        return escaped;
    }

    std::wstring CsvConfigService::FormatCsvRow(TouchpadPhysicalSizeEntry const& entry)
    {
        return EscapeCsvField(entry.identity.manufacturer) + L"," +
            EscapeCsvField(entry.identity.productName) + L"," +
            EscapeCsvField(entry.identity.sku) + L"," +
            EscapeCsvField(entry.identity.version) + L"," +
            FormatMm(entry.widthMm) + L"," +
            FormatMm(entry.heightMm);
    }

    bool CsvConfigService::UpsertEntry(TouchpadPhysicalSizeEntry const& entry)
    {
        if (entry.widthMm <= 0.0 || entry.heightMm <= 0.0)
        {
            Logger::Error(L"Cannot export invalid touchpad size");
            return false;
        }

        try
        {
            std::filesystem::create_directories(m_csvPath.parent_path());
        }
        catch (...)
        {
            Logger::Error(L"Failed to create config directory for TouchpadPhysicalSize.csv");
            return false;
        }

        std::vector<TouchpadPhysicalSizeEntry> rows;
        if (std::filesystem::exists(m_csvPath))
        {
            std::wifstream input(m_csvPath);
            if (!input)
            {
                Logger::Error(L"Failed to read TouchpadPhysicalSize.csv: " + m_csvPath.wstring());
                return false;
            }

            std::wstring line;
            bool headerSkipped = false;
            while (std::getline(input, line))
            {
                if (line.empty())
                {
                    continue;
                }

                if (!headerSkipped)
                {
                    headerSkipped = true;
                    continue;
                }

                if (auto parsed = ParseEntryRow(SplitCsvLine(line)))
                {
                    if (!IdentityEquals(parsed->identity, entry.identity))
                    {
                        rows.push_back(*parsed);
                    }
                }
            }
        }

        rows.push_back(entry);

        std::wofstream output(m_csvPath, std::ios::trunc);
        if (!output)
        {
            Logger::Error(L"Failed to write TouchpadPhysicalSize.csv: " + m_csvPath.wstring());
            return false;
        }

        output << kHeaderLine << L"\n";
        for (auto const& row : rows)
        {
            output << FormatCsvRow(row) << L"\n";
        }

        Logger::Info(L"Exported touchpad physical size to " + m_csvPath.wstring());
        return true;
    }

    std::optional<TouchpadPhysicalSizeEntry> CsvConfigService::Match(BiosIdentity const& identity) const
    {
        if (!std::filesystem::exists(m_csvPath))
        {
            Logger::Error(L"TouchpadPhysicalSize.csv not found: " + m_csvPath.wstring());
            return std::nullopt;
        }

        std::wifstream stream(m_csvPath);
        if (!stream)
        {
            return std::nullopt;
        }

        std::wstring line;
        bool headerSkipped = false;
        while (std::getline(stream, line))
        {
            if (line.empty())
            {
                continue;
            }

            if (!headerSkipped)
            {
                headerSkipped = true;
                continue;
            }

            const auto columns = SplitCsvLine(line);
            if (auto parsed = ParseEntryRow(columns))
            {
                if (IdentityEquals(parsed->identity, identity))
                {
                    return parsed;
                }
            }
        }

        return std::nullopt;
    }

    std::vector<std::wstring> CsvConfigService::SplitCsvLine(std::wstring const& line)
    {
        std::vector<std::wstring> parts;
        std::wstring current;
        for (wchar_t ch : line)
        {
            if (ch == L',')
            {
                parts.push_back(Trim(current));
                current.clear();
            }
            else
            {
                current.push_back(ch);
            }
        }
        parts.push_back(Trim(current));
        return parts;
    }
}
