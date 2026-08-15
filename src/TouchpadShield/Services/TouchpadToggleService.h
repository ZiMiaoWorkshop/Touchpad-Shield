#pragma once

#include <atomic>

namespace TouchpadShield::Services
{
    class TouchpadToggleService
    {
    public:
        static constexpr int TouchpadToggleVerifyDelayMs = 500;
        static constexpr int TouchpadToggleMaxAttempts = 2;

        winrt::Windows::Foundation::IAsyncAction RequestEnabledAsync(bool wantEnabled);

    private:
        bool SendToggleInput() const;
        winrt::Windows::Foundation::IAsyncAction VerifyAndCompensateAsync(bool wantEnabled, int attempt);

        std::atomic<bool> m_toggleInProgress{ false };
    };
}
