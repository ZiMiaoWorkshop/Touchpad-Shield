#include "pch.h"
#include "Services/Logger.h"

#include <ShlObj.h>
#include <sstream>

std::mutex Logger::s_mutex{};

std::wstring Logger::LogFilePath()
{
#ifdef TOUCHPAD_SHIELD_DEBUG
    wchar_t localAppData[MAX_PATH]{};
    if (SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, localAppData) == S_OK)
    {
        std::filesystem::path folder = std::filesystem::path(localAppData) / L"TouchpadShield" / L"logs";
        std::error_code ec;
        std::filesystem::create_directories(folder, ec);
        return (folder / L"touchpad-shield.log").wstring();
    }
#endif
    return L"";
}

void Logger::Write(std::wstring const& level, std::wstring const& message)
{
    std::lock_guard lock(s_mutex);

    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
    localtime_s(&localTime, &time);

    std::wostringstream line;
    line << L"[" << std::put_time(&localTime, L"%Y-%m-%d %H:%M:%S") << L"][" << level << L"] " << message;

#ifdef TOUCHPAD_SHIELD_DEBUG
    OutputDebugStringW((line.str() + L"\n").c_str());
    const auto path = LogFilePath();
    if (!path.empty())
    {
        std::wofstream stream(path, std::ios::app);
        if (stream)
        {
            stream << line.str() << L'\n';
        }
    }
#else
    (void)level;
    (void)message;
#endif
}

void Logger::Info(std::wstring const& message)
{
    Write(L"INFO", message);
}

void Logger::Warning(std::wstring const& message)
{
    Write(L"WARN", message);
}

void Logger::Error(std::wstring const& message)
{
    Write(L"ERROR", message);
}
