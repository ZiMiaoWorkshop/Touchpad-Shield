#pragma once

#include <mutex>
#include <string>

class Logger
{
public:
    static void Info(std::wstring const& message);
    static void Warning(std::wstring const& message);
    static void Error(std::wstring const& message);

private:
    static void Write(std::wstring const& level, std::wstring const& message);
    static std::wstring LogFilePath();
    static std::mutex s_mutex;
};
