#include "stdafx.h"
#include "UI/NewUI/HUD/MainFrameGauges.h"

#include <algorithm>

#include "UI/NewUI/NewUICommon.h"

namespace UI::MainFrame::Gauges
{

namespace
{
// Below this the visible slice is thinner than a texel; drawing it would sample
// a negative source extent.
constexpr float kMinimumVisibleRatio = 0.01f;
}

void RenderOrb(int emptyImage, int fullImage, const Layout::Rect& rect, ArtSize art, float fillRatio)
{
    SEASON3B::RenderImageStretch(emptyImage, rect.x, rect.y, rect.width, rect.height, 0.0f, 0.0f, art.width,
                                 art.height);

    if (fillRatio < kMinimumVisibleRatio)
        return;

    const float emptyRatio = 1.0f - fillRatio;
    SEASON3B::RenderImageStretch(fullImage, rect.x, rect.y + rect.height * emptyRatio, rect.width,
                                 rect.height * fillRatio, 0.0f, art.height * emptyRatio, art.width,
                                 art.height * fillRatio);
}

void RenderHorizontal(int image, const Layout::Rect& rect, ArtSize art, float fillRatio)
{
    if (fillRatio < kMinimumVisibleRatio)
        return;

    SEASON3B::RenderImageStretch(image, rect.x, rect.y, rect.width * fillRatio, rect.height, 0.0f, 0.0f,
                                 art.width * fillRatio, art.height);
}

float FillRatio(double value, double max)
{
    if (max <= 0.0)
        return 0.0f;

    return static_cast<float>(std::clamp(value / max, 0.0, 1.0));
}

}
