#pragma once

#include "Services/RegistryService.h"

#include <string>
#include <vector>

namespace winrt::Microsoft::UI::Xaml::Controls
{
    struct Canvas;
}

namespace winrt::Microsoft::UI::Xaml::Media
{
    struct SolidColorBrush;
}

namespace TouchpadShield::Services
{
    struct DiagramRenderInput
    {
        double touchpadWidthMm{ 65.0 };
        double touchpadHeightMm{ 40.0 };
        EdgeValues curtain{};
        EdgeValues superCurtain{};
        double mmPerDip{ 25.4 / 96.0 };
    };

    struct DiagramRenderResult
    {
        std::vector<std::wstring> overlapEdgeLabels;
    };

    class TouchpadDiagramRenderer
    {
    public:
        DiagramRenderResult Render(
            winrt::Microsoft::UI::Xaml::Controls::Canvas const& canvas,
            DiagramRenderInput const& input) const;

    private:
        winrt::Microsoft::UI::Xaml::Media::SolidColorBrush MakeBrush(
            winrt::Windows::UI::Color color,
            double opacity) const;
    };
}
