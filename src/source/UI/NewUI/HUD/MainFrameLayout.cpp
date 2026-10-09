#include "stdafx.h"
#include "UI/NewUI/HUD/MainFrameLayout.h"

#include <array>

namespace UI::MainFrame::Layout
{

namespace
{
constexpr float kReferenceWidth = 640.0f;
constexpr float kReferenceHeight = 480.0f;

// MenuS8_Main texel size. The frame is stretched uniformly to the reference width.
constexpr float kFrameTexelWidth = 935.0f;
constexpr float kFrameTexelHeight = 110.0f;

// The ring holes are circles of ~41 texels radius; the orb art is drawn a little
// larger so its rim tucks under the ring instead of leaving a gap.
constexpr float kOrbTexelSize = 84.0f;
constexpr Rect kLifeOrbTexels{160.5f, 5.0f, kOrbTexelSize, kOrbTexelSize};
constexpr Rect kManaOrbTexels{681.0f, 5.5f, kOrbTexelSize, kOrbTexelSize};

constexpr Rect kShieldGaugeTexels{256.0f, 32.0f, 102.0f, 14.0f};
constexpr Rect kSkillManaGaugeTexels{578.0f, 32.0f, 101.0f, 14.0f};
constexpr Rect kExperienceBarTexels{90.0f, 100.0f, 755.0f, 6.0f};
// Space left of the "%" sign baked into the green pill.
constexpr Rect kExperiencePercentTexels{46.0f, 97.0f, 30.0f, 12.0f};

constexpr float kItemSlotTexelLeft = 268.5f;
constexpr float kItemSlotTexelPitch = 42.6f;
constexpr float kItemSlotTexelTop = 57.0f;
constexpr float kItemSlotTexelWidth = 39.0f;
constexpr float kItemSlotTexelHeight = 36.0f;

constexpr Rect kCurrentSkillSlotTexels{448.0f, 55.0f, 30.0f, 38.0f};

constexpr float kSkillSlotTexelLeft = 486.0f;
constexpr float kSkillSlotTexelPitch = 32.6f;
constexpr float kSkillSlotTexelTop = 55.0f;
constexpr float kSkillSlotTexelWidth = 28.0f;
constexpr float kSkillSlotTexelHeight = 38.0f;

// Button art is 27x28; it is drawn slightly larger to fill the round sockets.
constexpr float kButtonTexelWidth = 29.0f;
constexpr float kButtonTexelHeight = 30.0f;
constexpr float kButtonTexelCenterY = 80.5f;
constexpr std::array<float, static_cast<size_t>(MenuButton::Count)> kButtonTexelCentersX = {
    79.0f, 114.0f, 149.0f, 786.0f, 821.0f, 857.0f,
};

// Opaque areas used for hit testing: the slot band across the whole bar and the
// gauge row above it, the highest part of the bar besides the orbs (added separately).
constexpr float kSlotBandTexelTop = 46.0f;
constexpr float kGaugeRowTexelTop = 26.0f;
constexpr Rect kSlotBandTexels{0.0f, kSlotBandTexelTop, kFrameTexelWidth, kFrameTexelHeight - kSlotBandTexelTop};
constexpr Rect kGaugeRowTexels{245.0f, kGaugeRowTexelTop, 445.0f, kSlotBandTexelTop - kGaugeRowTexelTop};

float TexelScale()
{
    return kReferenceWidth / kFrameTexelWidth;
}

float FrameTop()
{
    return kReferenceHeight - kFrameTexelHeight * TexelScale();
}

Rect FromTexels(const Rect& texels)
{
    const float scale = TexelScale();
    return {texels.x * scale, FrameTop() + texels.y * scale, texels.width * scale, texels.height * scale};
}
}

bool Rect::Contains(float pointX, float pointY) const
{
    return pointX >= x && pointX < x + width && pointY >= y && pointY < y + height;
}

float Rect::CenterX() const
{
    return x + width * 0.5f;
}

float Rect::CenterY() const
{
    return y + height * 0.5f;
}

Rect Frame()
{
    return FromTexels({0.0f, 0.0f, kFrameTexelWidth, kFrameTexelHeight});
}

Rect LifeOrb()
{
    return FromTexels(kLifeOrbTexels);
}

Rect ManaOrb()
{
    return FromTexels(kManaOrbTexels);
}

Rect ShieldGauge()
{
    return FromTexels(kShieldGaugeTexels);
}

Rect SkillManaGauge()
{
    return FromTexels(kSkillManaGaugeTexels);
}

Rect ExperienceBar()
{
    return FromTexels(kExperienceBarTexels);
}

Rect ExperiencePercent()
{
    return FromTexels(kExperiencePercentTexels);
}

Rect ItemHotKeySlot(int index)
{
    const float left = kItemSlotTexelLeft + static_cast<float>(index) * kItemSlotTexelPitch;
    return FromTexels({left, kItemSlotTexelTop, kItemSlotTexelWidth, kItemSlotTexelHeight});
}

Rect CurrentSkillSlot()
{
    return FromTexels(kCurrentSkillSlotTexels);
}

Rect SkillHotKeySlot(int index)
{
    const float left = kSkillSlotTexelLeft + static_cast<float>(index) * kSkillSlotTexelPitch;
    return FromTexels({left, kSkillSlotTexelTop, kSkillSlotTexelWidth, kSkillSlotTexelHeight});
}

Rect SkillHotKeyStrip()
{
    const Rect first = SkillHotKeySlot(0);
    const Rect last = SkillHotKeySlot(SkillHotKeySlotCount - 1);
    return {first.x, first.y, last.x + last.width - first.x, first.height};
}

Rect MenuButtonRect(MenuButton button)
{
    const float centerX = kButtonTexelCentersX[static_cast<size_t>(button)];
    return FromTexels({centerX - kButtonTexelWidth * 0.5f, kButtonTexelCenterY - kButtonTexelHeight * 0.5f,
                       kButtonTexelWidth, kButtonTexelHeight});
}

float SkillListBottom()
{
    return FrameTop() + kGaugeRowTexelTop * TexelScale();
}

bool ContainsPoint(float x, float y)
{
    return FromTexels(kSlotBandTexels).Contains(x, y) || FromTexels(kGaugeRowTexels).Contains(x, y)
           || LifeOrb().Contains(x, y) || ManaOrb().Contains(x, y);
}

}
