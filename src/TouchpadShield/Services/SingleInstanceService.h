#pragma once

namespace TouchpadShield::Services
{
    class SingleInstanceService
    {
    public:
        bool TryAcquire();
        void Release();
        bool ActivateExistingInstance();
        static UINT ActivateMainWindowMessage();

    private:
        static constexpr wchar_t kMutexName[] = L"Local\\TouchpadShield_SingleInstance_v2";

        void* m_mutexHandle{ nullptr };
    };

    inline constexpr wchar_t kMainWindowTitle[] = L"Touchpad Shield";
}
