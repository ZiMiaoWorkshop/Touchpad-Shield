#include "pch.h"
#include "Services/WindowIconHelper.h"
#include "resource.h"

namespace TouchpadShield::Services
{
    namespace
    {
        HICON LoadIconFromFile(std::filesystem::path const& path, int width, int height)
        {
            return static_cast<HICON>(LoadImageW(
                nullptr,
                path.c_str(),
                IMAGE_ICON,
                width,
                height,
                LR_LOADFROMFILE));
        }

        HICON LoadIconFromResource(HINSTANCE instance, wchar_t const* resourceId, int width, int height)
        {
            return static_cast<HICON>(LoadImageW(
                instance,
                resourceId,
                IMAGE_ICON,
                width,
                height,
                LR_DEFAULTCOLOR));
        }

        std::filesystem::path ResolveIconPath()
        {
            wchar_t modulePath[MAX_PATH]{};
            if (GetModuleFileNameW(nullptr, modulePath, MAX_PATH) == 0)
            {
                return {};
            }

            const auto baseDir = std::filesystem::path(modulePath).parent_path();
            const std::filesystem::path candidates[] = {
                baseDir / L"Assets" / L"TouchpadShield.ico",
                baseDir / L"TouchpadShield.ico",
            };

            for (auto const& candidate : candidates)
            {
                if (std::filesystem::exists(candidate))
                {
                    return candidate;
                }
            }

            return {};
        }
    }

    void WindowIconHelper::Apply(HWND hwnd)
    {
        if (!hwnd)
        {
            return;
        }

        HICON largeIcon = nullptr;
        HICON smallIcon = nullptr;

        if (auto iconPath = ResolveIconPath(); !iconPath.empty())
        {
            largeIcon = LoadIconFromFile(iconPath, 32, 32);
            smallIcon = LoadIconFromFile(iconPath, 16, 16);
        }

        if (!largeIcon)
        {
            largeIcon = LoadIconFromResource(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_APPICON), 32, 32);
        }
        if (!smallIcon)
        {
            smallIcon = LoadIconFromResource(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_APPICON), 16, 16);
        }

        if (largeIcon)
        {
            SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(largeIcon));
        }
        if (smallIcon)
        {
            SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(smallIcon));
        }
    }
}
