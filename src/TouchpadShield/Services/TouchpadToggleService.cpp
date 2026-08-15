#include "pch.h"
#include "Services/TouchpadToggleService.h"
#include "Services/TouchpadStatusService.h"

namespace TouchpadShield::Services
{
    namespace
    {
        std::wstring EnabledText(std::optional<bool> const& value)
        {
            if (!value.has_value())
            {
                return L"unknown";
            }
            return value.value() ? L"1" : L"0";
        }
    }

    bool TouchpadToggleService::SendToggleInput() const
    {
        INPUT inputs[6]{};

        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = VK_LCONTROL;
        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = VK_LWIN;
        inputs[2].type = INPUT_KEYBOARD;
        inputs[2].ki.wVk = VK_F24;

        inputs[3].type = INPUT_KEYBOARD;
        inputs[3].ki.wVk = VK_F24;
        inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
        inputs[4].type = INPUT_KEYBOARD;
        inputs[4].ki.wVk = VK_LWIN;
        inputs[4].ki.dwFlags = KEYEVENTF_KEYUP;
        inputs[5].type = INPUT_KEYBOARD;
        inputs[5].ki.wVk = VK_LCONTROL;
        inputs[5].ki.dwFlags = KEYEVENTF_KEYUP;

        const UINT sent = SendInput(static_cast<UINT>(std::size(inputs)), inputs, sizeof(INPUT));
        if (sent != std::size(inputs))
        {
            Logger::Error(L"SendInput F24 failed");
            return false;
        }

        Logger::Info(L"SendInput Ctrl+Win+F24 sent");
        return true;
    }

    winrt::Windows::Foundation::IAsyncAction TouchpadToggleService::VerifyAndCompensateAsync(
        bool wantEnabled,
        int attempt)
    {
        co_await winrt::resume_after(std::chrono::milliseconds(TouchpadToggleVerifyDelayMs));

        TouchpadStatusService statusService{};
        const auto current = statusService.IsEnabled();
        const bool matches = current.has_value() && current.value() == wantEnabled;
        if (matches)
        {
            Logger::Info(
                L"Touchpad toggle verified: Enabled=" + EnabledText(current) +
                L" attempt=" + std::to_wstring(attempt + 1));
            co_return;
        }

        if (attempt + 1 >= TouchpadToggleMaxAttempts)
        {
            Logger::Warning(
                L"Touchpad toggle failed after max attempts; want Enabled=" +
                std::wstring(wantEnabled ? L"1" : L"0") + L" got=" + EnabledText(current));
            co_return;
        }

        const bool shouldSend =
            current.has_value() &&
            ((wantEnabled && !current.value()) || (!wantEnabled && current.value()));
        if (!shouldSend)
        {
            Logger::Info(L"Touchpad toggle compensation skipped; state guard blocked resend");
            co_return;
        }

        Logger::Info(L"Touchpad toggle compensating; attempt=" + std::to_wstring(attempt + 2));
        if (SendToggleInput())
        {
            co_await VerifyAndCompensateAsync(wantEnabled, attempt + 1);
        }
    }

    winrt::Windows::Foundation::IAsyncAction TouchpadToggleService::RequestEnabledAsync(bool wantEnabled)
    {
        if (m_toggleInProgress.exchange(true))
        {
            Logger::Info(L"Touchpad toggle skipped; operation already in progress");
            co_return;
        }

        struct ResetGuard
        {
            std::atomic<bool>& flag;
            ~ResetGuard() { flag.store(false); }
        } reset{ m_toggleInProgress };

        TouchpadStatusService statusService{};
        const auto current = statusService.IsEnabled();
        if (!current.has_value())
        {
            Logger::Warning(L"Touchpad toggle skipped; Status\\Enabled unavailable");
            co_return;
        }

        if (current.value() == wantEnabled)
        {
            Logger::Info(
                L"Touchpad toggle skipped; already Enabled=" + EnabledText(current));
            co_return;
        }

        const bool shouldSend =
            (wantEnabled && !current.value()) || (!wantEnabled && current.value());
        if (!shouldSend)
        {
            co_return;
        }

        if (!SendToggleInput())
        {
            co_return;
        }

        co_await VerifyAndCompensateAsync(wantEnabled, 0);
    }
}
