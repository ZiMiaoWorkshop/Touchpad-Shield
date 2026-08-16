#include "pch.h"
#include "Services/InputDeviceTypes.h"
#include "Services/StringUtils.h"

namespace TouchpadShield::Services
{
    namespace
    {
        std::wstring NormalizeLabelKey(std::wstring const& label)
        {
            return ToLower(Trim(label));
        }

        std::wstring NormalizeModelIdKey(std::wstring modelId)
        {
            modelId = Trim(ToUpper(std::move(modelId)));
            if (modelId.empty())
            {
                return L"";
            }

            int slashCount = 0;
            for (size_t i = 0; i < modelId.size(); ++i)
            {
                if (modelId[i] != L'\\')
                {
                    continue;
                }

                ++slashCount;
                if (slashCount >= 2)
                {
                    return modelId.substr(0, i);
                }
            }

            return modelId;
        }
    }

    std::wstring ContainerIdToString(GUID const& containerId)
    {
        wchar_t buffer[64] = {};
        if (StringFromGUID2(containerId, buffer, static_cast<int>(std::size(buffer))) <= 0)
        {
            return L"";
        }
        return buffer;
    }

    bool TryParseContainerId(std::wstring const& text, GUID& containerId)
    {
        if (text.empty())
        {
            return false;
        }

        HRESULT hr = CLSIDFromString(text.c_str(), &containerId);
        return SUCCEEDED(hr);
    }

    std::wstring BuildDeviceMatchKey(std::wstring const& label, std::wstring const& modelId)
    {
        const std::wstring normalizedModelId = NormalizeModelIdKey(modelId);
        if (!normalizedModelId.empty())
        {
            return L"model:" + normalizedModelId;
        }

        const std::wstring normalizedLabel = NormalizeLabelKey(label);
        if (!normalizedLabel.empty())
        {
            return L"label:" + normalizedLabel;
        }

        return L"";
    }

    bool InputDevicesMatch(MonitoredInputDevice const& monitored, InputDeviceInfo const& online)
    {
        if (IsEqualGUID(monitored.containerId, online.containerId) != FALSE)
        {
            return true;
        }

        if (!monitored.matchKey.empty() &&
            !online.matchKey.empty() &&
            monitored.matchKey == online.matchKey)
        {
            return true;
        }

        return false;
    }
}
