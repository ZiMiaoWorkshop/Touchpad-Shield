#pragma once

namespace TouchpadShield::Services
{
    class SingleInstanceService
    {
    public:
        bool TryAcquire();
        void Release();
        bool ActivateExistingInstance();

    private:
        static constexpr wchar_t kMutexName[] = L"Global\\TouchpadShield_SingleInstance_v1";
        static constexpr wchar_t kWindowTitle[] = L"Touchpad Shield";

        void* m_mutexHandle{ nullptr };
    };

    inline constexpr wchar_t kMainWindowTitle[] = L"Touchpad Shield";
}
