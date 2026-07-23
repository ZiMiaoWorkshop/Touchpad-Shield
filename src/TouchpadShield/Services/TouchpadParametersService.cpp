#include "pch.h"

#include "Services/TouchpadParametersService.h"

#include "Services/RegistryUserContext.h"

#include <functional>



namespace TouchpadShield::Services

{

    namespace

    {

#ifndef SPI_GETTOUCHPADPARAMETERS

        constexpr UINT kSpiGetTouchpadParameters = 0x00AE;

        constexpr UINT kSpiSetTouchpadParameters = 0x00AF;

#else

        constexpr UINT kSpiGetTouchpadParameters = SPI_GETTOUCHPADPARAMETERS;

        constexpr UINT kSpiSetTouchpadParameters = SPI_SETTOUCHPADPARAMETERS;

#endif



#ifndef TOUCHPAD_PARAMETERS_VERSION_1

        constexpr UINT kTouchpadParametersVersion1 = 1;



        enum TOUCHPAD_SENSITIVITY_LEVEL_LOCAL

        {

            TOUCHPAD_SENSITIVITY_LEVEL_MOST_SENSITIVE = 0,

            TOUCHPAD_SENSITIVITY_LEVEL_HIGH_SENSITIVITY = 1,

            TOUCHPAD_SENSITIVITY_LEVEL_MEDIUM_SENSITIVITY = 2,

            TOUCHPAD_SENSITIVITY_LEVEL_LOW_SENSITIVITY = 3,

            TOUCHPAD_SENSITIVITY_LEVEL_LEAST_SENSITIVE = 4,

        };



        struct TOUCHPAD_PARAMETERS_V1_LOCAL

        {

            UINT versionNumber;

            UINT maxSupportedContacts;

            UINT legacyTouchpadFeatures;

            BOOL touchpadPresent : 1;

            BOOL legacyTouchpadPresent : 1;

            BOOL externalMousePresent : 1;

            BOOL touchpadEnabled : 1;

            BOOL touchpadActive : 1;

            BOOL feedbackSupported : 1;

            BOOL clickForceSupported : 1;

            BOOL Reserved1 : 25;

            BOOL allowActiveWhenMousePresent : 1;

            BOOL feedbackEnabled : 1;

            BOOL tapEnabled : 1;

            BOOL tapAndDragEnabled : 1;

            BOOL twoFingerTapEnabled : 1;

            BOOL rightClickZoneEnabled : 1;

            BOOL mouseAccelSettingHonored : 1;

            BOOL panEnabled : 1;

            BOOL zoomEnabled : 1;

            BOOL scrollDirectionReversed : 1;

            BOOL Reserved2 : 22;

            TOUCHPAD_SENSITIVITY_LEVEL_LOCAL sensitivityLevel;

            UINT cursorSpeed;

            UINT feedbackIntensity;

            UINT clickForceSensitivity;

            UINT rightClickZoneWidth;

            UINT rightClickZoneHeight;

        };

#else

        constexpr UINT kTouchpadParametersVersion1 = TOUCHPAD_PARAMETERS_VERSION_1;

        using TouchpadParameters = TOUCHPAD_PARAMETERS_V1;

#endif



#ifndef TOUCHPAD_PARAMETERS_VERSION_1

        using TouchpadParameters = TOUCHPAD_PARAMETERS_V1_LOCAL;

#endif



        constexpr UINT kSpiUpdateIniFile = 0x01;

        constexpr UINT kSpiSendChange = 0x02;



        bool TryGetTouchpadParameters(TouchpadParameters& params)

        {

            params = {};

            params.versionNumber = kTouchpadParametersVersion1;

            return SystemParametersInfoW(

                       kSpiGetTouchpadParameters,

                       sizeof(params),

                       &params,

                       0) != FALSE;

        }



        bool TrySetTouchpadParameters(TouchpadParameters const& params)

        {

            TouchpadParameters mutableParams = params;

            mutableParams.versionNumber = kTouchpadParametersVersion1;

            return SystemParametersInfoW(

                       kSpiSetTouchpadParameters,

                       sizeof(mutableParams),

                       &mutableParams,

                       kSpiUpdateIniFile | kSpiSendChange) != FALSE;

        }



        bool ApplyTouchpadChange(std::function<bool(TouchpadParameters&)> const& mutator)

        {

            bool success = false;



            auto action = [&]() -> bool

            {

                TouchpadParameters params{};

                if (!TryGetTouchpadParameters(params))

                {

                    Logger::Info(L"SPI_GETTOUCHPADPARAMETERS unavailable, error=" + std::to_wstring(GetLastError()));

                    return false;

                }



                if (!mutator(params))

                {

                    return false;

                }



                if (!TrySetTouchpadParameters(params))

                {

                    Logger::Error(L"SPI_SETTOUCHPADPARAMETERS failed, error=" + std::to_wstring(GetLastError()));

                    return false;

                }



                Logger::Info(L"Touchpad settings applied via SystemParametersInfo");

                return true;

            };



            success = RunAsInteractiveUser(action);

            return success;

        }

        std::optional<TouchpadParameters> ReadTouchpadParameters()
        {
            std::optional<TouchpadParameters> result;
            RunAsInteractiveUser([&]() -> bool
            {
                TouchpadParameters params{};
                if (!TryGetTouchpadParameters(params))
                {
                    Logger::Info(L"SPI_GETTOUCHPADPARAMETERS unavailable, error=" + std::to_wstring(GetLastError()));
                    return false;
                }

                result = params;
                return true;
            });
            return result;
        }

    }



    std::optional<uint32_t> TouchpadParametersService::GetClickForceSensitivity()
    {
        const auto params = ReadTouchpadParameters();
        if (!params.has_value())
        {
            return std::nullopt;
        }

        return std::min<uint32_t>(params->clickForceSensitivity, 100);
    }

    std::optional<uint32_t> TouchpadParametersService::GetAAPThreshold()
    {
        const auto params = ReadTouchpadParameters();
        if (!params.has_value())
        {
            return std::nullopt;
        }

        return std::min<uint32_t>(static_cast<uint32_t>(params->sensitivityLevel), 3);
    }

    std::optional<bool> TouchpadParametersService::GetTapsEnabled()
    {
        const auto params = ReadTouchpadParameters();
        if (!params.has_value())
        {
            return std::nullopt;
        }

        return params->tapEnabled != FALSE;
    }



    bool TouchpadParametersService::SetClickForceSensitivity(uint32_t value)

    {

        value = std::min<uint32_t>(value, 100);

        return ApplyTouchpadChange([&](TouchpadParameters& params) -> bool

        {

            params.clickForceSensitivity = value;

            return true;

        });

    }



    bool TouchpadParametersService::SetAAPThreshold(uint32_t value)

    {

        value = std::min<uint32_t>(value, 4);

        return ApplyTouchpadChange([&](TouchpadParameters& params) -> bool

        {

            params.sensitivityLevel = static_cast<decltype(params.sensitivityLevel)>(value);

            return true;

        });

    }



    bool TouchpadParametersService::SetTapsEnabled(bool enabled)

    {

        return ApplyTouchpadChange([&](TouchpadParameters& params) -> bool

        {

            params.tapEnabled = enabled ? TRUE : FALSE;

            return true;

        });

    }

}


