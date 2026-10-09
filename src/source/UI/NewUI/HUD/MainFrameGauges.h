#pragma once

#include "UI/NewUI/HUD/MainFrameLayout.h"

namespace UI::MainFrame::Gauges
{
    // Size in texels of the source art a gauge samples from.
    struct ArtSize
    {
        float width;
        float height;
    };

    // Draws an orb that drains from the top: the empty art covers the whole rect
    // and the full art covers only its bottom fillRatio (0..1).
    void RenderOrb(int emptyImage, int fullImage, const Layout::Rect& rect, ArtSize art, float fillRatio);

    // Draws a bar that fills from the left. The art is cropped to the filled part
    // rather than squeezed, so its shading stays in place as the value changes.
    void RenderHorizontal(int image, const Layout::Rect& rect, ArtSize art, float fillRatio);

    // Fraction of max that value represents, clamped to 0..1 (0 when max is 0).
    float FillRatio(double value, double max);
}
