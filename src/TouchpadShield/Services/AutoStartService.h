#pragma once

namespace TouchpadShield::Services
{
    class AutoStartService
    {
    public:
        static constexpr wchar_t kRunValueName[] = L"TouchpadShield";
        static constexpr wchar_t kStartupArgument[] = L"--startup";

        bool IsEnabled() const;
        bool SetEnabled(bool enabled) const;
        std::wstring ResolveExecutablePath() const;
        std::wstring BuildRunCommand() const;
        static bool IsStartupLaunch();

    private:
        static constexpr wchar_t kRunKeyPath[] =
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    };
}
