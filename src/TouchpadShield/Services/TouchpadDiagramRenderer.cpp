#include "pch.h"
#include "Services/TouchpadDiagramRenderer.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Shapes;

namespace TouchpadShield::Services
{
    SolidColorBrush TouchpadDiagramRenderer::MakeBrush(Windows::UI::Color color, double opacity) const
    {
        color.A = static_cast<uint8_t>(255 * opacity);
        SolidColorBrush brush{};
        brush.Color(color);
        return brush;
    }

    DiagramRenderResult TouchpadDiagramRenderer::Render(Canvas const& canvas, DiagramRenderInput const& input) const
    {
        canvas.Children().Clear();

        DiagramRenderResult result{};
        const double mmPerDip = input.mmPerDip > 0.0 ? input.mmPerDip : (25.4 / 96.0);
        double widthPx = input.touchpadWidthMm / mmPerDip;
        double heightPx = input.touchpadHeightMm / mmPerDip;

        double canvasWidth = canvas.ActualWidth();
        double canvasHeight = canvas.ActualHeight();
        if (canvasWidth < 1.0)
        {
            canvasWidth = 600.0;
        }
        if (canvasHeight < 1.0)
        {
            canvasHeight = 380.0;
        }

        constexpr double canvasMargin = 8.0;
        const double availableWidth = canvasWidth - canvasMargin * 2.0;
        const double availableHeight = canvasHeight - canvasMargin * 2.0;
        const double fitScale = (std::min)(1.0, (std::min)(
            availableWidth / widthPx,
            availableHeight / heightPx));
        widthPx *= fitScale;
        heightPx *= fitScale;

        const double originX = (canvasWidth - widthPx) * 0.5;
        const double originY = (canvasHeight - heightPx) * 0.5;

        Shapes::Rectangle touchpad{};
        touchpad.Width(widthPx);
        touchpad.Height(heightPx);
        touchpad.Stroke(SolidColorBrush(Windows::UI::Color{ 255, 128, 128, 128 }));
        touchpad.StrokeThickness(1.5);
        touchpad.Fill(SolidColorBrush(Windows::UI::Color{ 0, 0, 0, 0 }));
        Canvas::SetLeft(touchpad, originX);
        Canvas::SetTop(touchpad, originY);
        canvas.Children().Append(touchpad);

        auto mmToPx = [&](double mm) -> double { return (mm / mmPerDip) * fitScale; };

        auto drawEdgeOverlay = [&](double top, double left, double overlayWidth, double overlayHeight, Windows::UI::Color color)
        {
            if (overlayWidth <= 0.0 || overlayHeight <= 0.0)
            {
                return;
            }

            Shapes::Rectangle overlay{};
            overlay.Width(overlayWidth);
            overlay.Height(overlayHeight);
            overlay.Fill(MakeBrush(color, 0.45));
            overlay.Stroke(SolidColorBrush(color));
            overlay.StrokeThickness(1.0);
            Canvas::SetLeft(overlay, originX + left);
            Canvas::SetTop(overlay, originY + top);
            canvas.Children().Append(overlay);
        };

        const Windows::UI::Color yellow{ 255, 255, 215, 0 };
        const Windows::UI::Color red{ 255, 220, 20, 60 };

        const double topCurtainPx = mmToPx(input.curtain.topMm);
        const double bottomCurtainPx = mmToPx(input.curtain.bottomMm);
        const double leftCurtainPx = mmToPx(input.curtain.leftMm);
        const double rightCurtainPx = mmToPx(input.curtain.rightMm);

        drawEdgeOverlay(0, 0, widthPx, topCurtainPx, yellow);
        drawEdgeOverlay(heightPx - bottomCurtainPx, 0, widthPx, bottomCurtainPx, yellow);
        drawEdgeOverlay(0, 0, leftCurtainPx, heightPx, yellow);
        drawEdgeOverlay(0, widthPx - rightCurtainPx, rightCurtainPx, heightPx, yellow);

        const double topSuperPx = mmToPx(input.superCurtain.topMm);
        const double bottomSuperPx = mmToPx(input.superCurtain.bottomMm);
        const double leftSuperPx = mmToPx(input.superCurtain.leftMm);
        const double rightSuperPx = mmToPx(input.superCurtain.rightMm);

        drawEdgeOverlay(0, 0, widthPx, topSuperPx, red);
        drawEdgeOverlay(heightPx - bottomSuperPx, 0, widthPx, bottomSuperPx, red);
        drawEdgeOverlay(0, 0, leftSuperPx, heightPx, red);
        drawEdgeOverlay(0, widthPx - rightSuperPx, rightSuperPx, heightPx, red);

        auto checkWarning = [&](std::wstring const& edge, double superMm, double curtainMm)
        {
            if (superMm >= curtainMm && superMm > 0.0 && curtainMm > 0.0)
            {
                result.warnings.push_back(edge + L"\xFF1A\x9632\x8BEF\x89E6\x533A\x57DF >= \x7F13\x51B2\x533A\x57DF\xFF0C\x8BE5\x533A\x57DF\x5C06\x5B8C\x5168\x4EE5\x9632\x8BEF\x89E6\x533A\x57DF\x903B\x8F91\x63A7\x5236\x3002");
            }
        };

        checkWarning(L"\x4E0A", input.superCurtain.topMm, input.curtain.topMm);
        checkWarning(L"\x4E0B", input.superCurtain.bottomMm, input.curtain.bottomMm);
        checkWarning(L"\x5DE6", input.superCurtain.leftMm, input.curtain.leftMm);
        checkWarning(L"\x53F3", input.superCurtain.rightMm, input.curtain.rightMm);

        return result;
    }
}
