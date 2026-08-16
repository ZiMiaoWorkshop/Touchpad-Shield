#include "pch.h"
#include "Services/AutoStartService.h"

#include <comdef.h>
#include <taskschd.h>

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsuppw.lib")

namespace TouchpadShield::Services
{
    namespace
    {
        constexpr HRESULT kTaskNotFoundHresult = 0x8004130F;
        constexpr wchar_t kAutostartHandledSessionKey[] = L"AutostartHandledSessionId";
        constexpr wchar_t kAppKeyPath[] = L"Software\\ZiMiaoWorkshop\\TouchpadShield";

        DWORD GetCurrentSessionId()
        {
            DWORD sessionId = 0;
            ProcessIdToSessionId(GetCurrentProcessId(), &sessionId);
            return sessionId;
        }

        std::optional<DWORD> ReadHandledAutostartSessionId()
        {
            HKEY key = nullptr;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, kAppKeyPath, 0, KEY_READ, &key) != ERROR_SUCCESS)
            {
                return std::nullopt;
            }

            DWORD value = 0;
            DWORD size = sizeof(value);
            DWORD type = REG_DWORD;
            const LSTATUS status = RegQueryValueExW(
                key,
                kAutostartHandledSessionKey,
                nullptr,
                &type,
                reinterpret_cast<LPBYTE>(&value),
                &size);
            RegCloseKey(key);

            if (status != ERROR_SUCCESS || type != REG_DWORD)
            {
                return std::nullopt;
            }

            return value;
        }

        void WriteHandledAutostartSessionId(DWORD sessionId)
        {
            HKEY key = nullptr;
            DWORD disposition = 0;
            if (RegCreateKeyExW(
                    HKEY_CURRENT_USER,
                    kAppKeyPath,
                    0,
                    nullptr,
                    0,
                    KEY_SET_VALUE,
                    nullptr,
                    &key,
                    &disposition) != ERROR_SUCCESS)
            {
                return;
            }

            RegSetValueExW(
                key,
                kAutostartHandledSessionKey,
                0,
                REG_DWORD,
                reinterpret_cast<const BYTE*>(&sessionId),
                sizeof(sessionId));
            RegCloseKey(key);
        }

        void ClearHandledAutostartSessionId()
        {
            HKEY key = nullptr;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, kAppKeyPath, 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS)
            {
                return;
            }

            RegDeleteValueW(key, kAutostartHandledSessionKey);
            RegCloseKey(key);
        }

        std::wstring GetCurrentUserSamName()
        {
            HANDLE token = nullptr;
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
            {
                return L"";
            }

            DWORD tokenInfoSize = 0;
            GetTokenInformation(token, TokenUser, nullptr, 0, &tokenInfoSize);
            std::vector<BYTE> tokenInfo(tokenInfoSize);
            if (!GetTokenInformation(token, TokenUser, tokenInfo.data(), tokenInfoSize, &tokenInfoSize))
            {
                CloseHandle(token);
                return L"";
            }
            CloseHandle(token);

            const auto* tokenUser = reinterpret_cast<TOKEN_USER*>(tokenInfo.data());
            wchar_t userName[256]{};
            wchar_t domainName[256]{};
            DWORD userNameSize = static_cast<DWORD>(std::size(userName));
            DWORD domainNameSize = static_cast<DWORD>(std::size(domainName));
            SID_NAME_USE sidType{};
            if (!LookupAccountSidW(
                    nullptr,
                    tokenUser->User.Sid,
                    userName,
                    &userNameSize,
                    domainName,
                    &domainNameSize,
                    &sidType))
            {
                return L"";
            }

            if (domainName[0] != L'\0')
            {
                return std::wstring(domainName) + L"\\" + userName;
            }

            return userName;
        }

        bool RunSchTasksDelete(DWORD* exitCodeOut = nullptr)
        {
            std::wstring commandLine =
                L"schtasks.exe /Delete /TN \"" + std::wstring(AutoStartService::kScheduledTaskName) + L"\" /F";

            STARTUPINFOW startupInfo{};
            startupInfo.cb = sizeof(startupInfo);
            startupInfo.dwFlags = STARTF_USESHOWWINDOW;
            startupInfo.wShowWindow = SW_HIDE;
            PROCESS_INFORMATION processInfo{};

            std::vector<wchar_t> buffer(commandLine.begin(), commandLine.end());
            buffer.push_back(L'\0');

            if (!CreateProcessW(
                    nullptr,
                    buffer.data(),
                    nullptr,
                    nullptr,
                    FALSE,
                    CREATE_NO_WINDOW,
                    nullptr,
                    nullptr,
                    &startupInfo,
                    &processInfo))
            {
                if (exitCodeOut)
                {
                    *exitCodeOut = static_cast<DWORD>(-1);
                }
                return false;
            }

            WaitForSingleObject(processInfo.hProcess, INFINITE);
            DWORD exitCode = 1;
            GetExitCodeProcess(processInfo.hProcess, &exitCode);
            CloseHandle(processInfo.hProcess);
            CloseHandle(processInfo.hThread);

            if (exitCodeOut)
            {
                *exitCodeOut = exitCode;
            }

            return exitCode == 0;
        }

        bool DeleteLogonTaskViaCom()
        {
            const HRESULT coinit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            const bool needsUninit = SUCCEEDED(coinit) && coinit != S_FALSE && coinit != RPC_E_CHANGED_MODE;

            ITaskService* service = nullptr;
            ITaskFolder* rootFolder = nullptr;
            HRESULT hr = CoCreateInstance(
                CLSID_TaskScheduler,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_ITaskService,
                reinterpret_cast<void**>(&service));

            if (FAILED(hr))
            {
                if (needsUninit)
                {
                    CoUninitialize();
                }
                return false;
            }

            hr = service->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
            if (FAILED(hr))
            {
                service->Release();
                if (needsUninit)
                {
                    CoUninitialize();
                }
                return false;
            }

            hr = service->GetFolder(_bstr_t(L"\\"), &rootFolder);
            if (FAILED(hr))
            {
                service->Release();
                if (needsUninit)
                {
                    CoUninitialize();
                }
                return false;
            }

            hr = rootFolder->DeleteTask(_bstr_t(AutoStartService::kScheduledTaskName), 0);
            rootFolder->Release();
            service->Release();

            if (needsUninit)
            {
                CoUninitialize();
            }

            return SUCCEEDED(hr) || hr == kTaskNotFoundHresult;
        }
    }

    bool AutoStartService::ShouldSkipStartupLaunch()
    {
        if (!IsStartupLaunch())
        {
            return false;
        }

        const DWORD sessionId = GetCurrentSessionId();
        const auto handledSessionId = ReadHandledAutostartSessionId();
        return handledSessionId.has_value() && handledSessionId.value() == sessionId;
    }

    void AutoStartService::MarkStartupLaunchHandled()
    {
        WriteHandledAutostartSessionId(GetCurrentSessionId());
    }

    std::wstring AutoStartService::ResolveExecutablePathUnquoted() const
    {
        wchar_t modulePath[MAX_PATH]{};
        const DWORD length = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
        if (length == 0 || length >= MAX_PATH)
        {
            return L"";
        }
        return modulePath;
    }

    bool AutoStartService::IsStartupLaunch()
    {
        int argc = 0;
        wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        if (!argv)
        {
            return false;
        }

        bool isStartup = false;
        for (int i = 1; i < argc; ++i)
        {
            if (_wcsicmp(argv[i], kStartupArgument) == 0)
            {
                isStartup = true;
                break;
            }
        }

        LocalFree(argv);
        return isStartup;
    }

    bool AutoStartService::RemoveRunKey() const
    {
        HKEY key = nullptr;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKeyPath, 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS)
        {
            return false;
        }

        LSTATUS status = RegDeleteValueW(key, kRunValueName);
        if (status == ERROR_FILE_NOT_FOUND)
        {
            status = ERROR_SUCCESS;
        }

        RegCloseKey(key);
        return status == ERROR_SUCCESS;
    }

    bool AutoStartService::CreateLogonTask() const
    {
        const std::wstring exePath = ResolveExecutablePathUnquoted();
        if (exePath.empty())
        {
            return false;
        }

        DeleteLogonTask();

        const std::wstring userSam = GetCurrentUserSamName();
        if (userSam.empty())
        {
            Logger::Warning(L"AutoStart: unable to resolve current user for logon trigger");
            return false;
        }

        const HRESULT coinit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        const bool needsUninit = SUCCEEDED(coinit) && coinit != S_FALSE && coinit != RPC_E_CHANGED_MODE;

        ITaskService* service = nullptr;
        ITaskFolder* rootFolder = nullptr;
        ITaskDefinition* task = nullptr;
        IRegisteredTask* registeredTask = nullptr;
        HRESULT hr = CoCreateInstance(
            CLSID_TaskScheduler,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_ITaskService,
            reinterpret_cast<void**>(&service));

        if (FAILED(hr))
        {
            Logger::Warning(L"AutoStart: TaskScheduler CoCreateInstance failed");
            if (needsUninit)
            {
                CoUninitialize();
            }
            return false;
        }

        hr = service->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
        if (FAILED(hr))
        {
            Logger::Warning(L"AutoStart: TaskScheduler Connect failed");
            service->Release();
            if (needsUninit)
            {
                CoUninitialize();
            }
            return false;
        }

        hr = service->GetFolder(_bstr_t(L"\\"), &rootFolder);
        if (FAILED(hr))
        {
            Logger::Warning(L"AutoStart: TaskScheduler GetFolder failed");
            service->Release();
            if (needsUninit)
            {
                CoUninitialize();
            }
            return false;
        }

        hr = service->NewTask(0, &task);
        if (FAILED(hr))
        {
            Logger::Warning(L"AutoStart: TaskScheduler NewTask failed");
            rootFolder->Release();
            service->Release();
            if (needsUninit)
            {
                CoUninitialize();
            }
            return false;
        }

        IRegistrationInfo* registrationInfo = nullptr;
        if (SUCCEEDED(task->get_RegistrationInfo(&registrationInfo)) && registrationInfo)
        {
            registrationInfo->put_Author(_bstr_t(L"Touchpad Shield"));
            registrationInfo->Release();
        }

        IPrincipal* principal = nullptr;
        if (SUCCEEDED(task->get_Principal(&principal)) && principal)
        {
            principal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
            principal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);
            principal->put_UserId(_bstr_t(userSam.c_str()));
            principal->Release();
        }

        ITaskSettings* settings = nullptr;
        if (SUCCEEDED(task->get_Settings(&settings)) && settings)
        {
            settings->put_StartWhenAvailable(VARIANT_TRUE);
            settings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
            settings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
            settings->put_MultipleInstances(TASK_INSTANCES_STOP_EXISTING);
            settings->Release();
        }

        ITriggerCollection* triggerCollection = nullptr;
        ITrigger* trigger = nullptr;
        ILogonTrigger* logonTrigger = nullptr;
        if (SUCCEEDED(task->get_Triggers(&triggerCollection)) &&
            SUCCEEDED(triggerCollection->Create(TASK_TRIGGER_LOGON, &trigger)) &&
            SUCCEEDED(trigger->QueryInterface(IID_ILogonTrigger, reinterpret_cast<void**>(&logonTrigger))))
        {
            logonTrigger->put_Id(_bstr_t(L"TouchpadShieldLogonTrigger"));
            logonTrigger->put_UserId(_bstr_t(userSam.c_str()));
            logonTrigger->Release();
        }

        if (trigger)
        {
            trigger->Release();
        }
        if (triggerCollection)
        {
            triggerCollection->Release();
        }

        IActionCollection* actionCollection = nullptr;
        IAction* action = nullptr;
        IExecAction* execAction = nullptr;
        if (SUCCEEDED(task->get_Actions(&actionCollection)) &&
            SUCCEEDED(actionCollection->Create(TASK_ACTION_EXEC, &action)) &&
            SUCCEEDED(action->QueryInterface(IID_IExecAction, reinterpret_cast<void**>(&execAction))))
        {
            execAction->put_Path(_bstr_t(exePath.c_str()));
            execAction->put_Arguments(_bstr_t(kStartupArgument));
            execAction->Release();
        }

        if (action)
        {
            action->Release();
        }
        if (actionCollection)
        {
            actionCollection->Release();
        }

        hr = rootFolder->RegisterTaskDefinition(
            _bstr_t(kScheduledTaskName),
            task,
            TASK_CREATE_OR_UPDATE,
            _variant_t(),
            _variant_t(),
            TASK_LOGON_INTERACTIVE_TOKEN,
            _variant_t(L""),
            &registeredTask);

        if (registeredTask)
        {
            registeredTask->Release();
        }
        task->Release();
        rootFolder->Release();
        service->Release();

        if (needsUninit)
        {
            CoUninitialize();
        }

        if (FAILED(hr))
        {
            Logger::Warning(L"AutoStart: RegisterTaskDefinition failed");
            return false;
        }

        Logger::Info(L"AutoStart: logon scheduled task registered for " + userSam);
        return true;
    }

    bool AutoStartService::DeleteLogonTask() const
    {
        const bool comDeleted = DeleteLogonTaskViaCom();

        DWORD exitCode = static_cast<DWORD>(-1);
        const bool schtasksDeleted = RunSchTasksDelete(&exitCode);

        return comDeleted || schtasksDeleted || exitCode == 1;
    }

    bool AutoStartService::SetEnabled(bool enabled) const
    {
        if (enabled)
        {
            const bool taskOk = CreateLogonTask();
            RemoveRunKey();
            return taskOk;
        }

        ClearHandledAutostartSessionId();
        DeleteLogonTask();
        return RemoveRunKey();
    }
}
