#pragma once

#include <functional>

namespace TouchpadShield::Services
{
    // Elevated processes may resolve HKCU to the wrong hive; run HKCU work under the interactive user.
    bool RunAsInteractiveUser(std::function<bool()> const& action);
    void NotifyPrecisionTouchPadSettingsChanged();
}
