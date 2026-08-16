#pragma once

namespace TouchpadShield::Services
{
    class AutoStartService
    {
    public:
        static constexpr wchar_t kRunValueName[] = L"TouchpadShield";
        static constexpr wchar_t kScheduledTaskName[] = L"TouchpadShield";
        static constexpr wchar_t kStartupArgument[] = L"--startup";

        bool SetEnabled(bool enabled) const;
        bool EnsureLogonTaskRegistered() const;
        static bool IsStartupLaunch();
        static bool ShouldSkipStartupLaunch();
        static void MarkStartupLaunchHandled();

    private:
        std::wstring ResolveExecutablePathUnquoted() const;
        bool CreateLogonTask() const;
        bool DeleteLogonTask() const;
        bool RemoveRunKey() const;

        static constexpr wchar_t kRunKeyPath[] =
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    };
}
