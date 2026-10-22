#include "stdafx.h"

#include "UI/NewUI/Quests/GoldenMotes.h"

#include "Render/Textures/ZzzOpenglUtil.h"

#include <cmath>
#include <random>

namespace UI::Quests
{
namespace
{
constexpr float Pi = 3.14159265f;
// The same seed every time, so that the motes always look the same.
constexpr unsigned int Seed = 242;

constexpr float SlowestRisesPerSecond = 0.12f;
constexpr float FastestRisesPerSecond = 0.25f;
constexpr float MaximumSway = 0.06f;
constexpr float SmallestRadius = 0.8f;
constexpr float BiggestRadius = 1.6f;
// How many times a mote sways to each side while it rises.
constexpr float SwaysPerRise = 1.5f;
// The motes stay away from the borders of the area.
constexpr float ColumnMargin = 0.08f;

// A round glow, which fades out to its border, around a brighter center.
constexpr unsigned int GlowColor = 0x00F0C25Au;
constexpr unsigned int CenterColor = 0x00FFF0C0u;
constexpr float GlowAlpha = 0.45f;
constexpr float CenterAlpha = 0.9f;
constexpr float GlowScale = 3.0f;
// The circles are drawn as this many triangles around their center.
constexpr int CircleSegments = 12;
constexpr int AlphaShift = 24;
constexpr float MaximumAlpha = 255.0f;

unsigned int WithAlpha(unsigned int rgb, float alpha)
{
    return rgb | (static_cast<unsigned int>(alpha * MaximumAlpha) << AlphaShift);
}

// A circle whose color goes from the one of its center to the one of its border.
void DrawCircle(float centerX, float centerY, float radius, unsigned int centerColor, unsigned int borderColor)
{
    for (int segment = 0; segment < CircleSegments; ++segment)
    {
        const float start = segment * 2 * Pi / CircleSegments;
        const float end = (segment + 1) * 2 * Pi / CircleSegments;
        const float endX = centerX + radius * std::cos(end);
        const float endY = centerY + radius * std::sin(end);

        // A triangle is a quad whose last two corners are the same.
        const float x[4] = {centerX, centerX + radius * std::cos(start), endX, endX};
        const float y[4] = {centerY, centerY + radius * std::sin(start), endY, endY};
        const unsigned int colors[4] = {centerColor, borderColor, borderColor, borderColor};
        RenderColorQuadPointsARGB(x, y, colors);
    }
}
} // namespace

GoldenMotes::GoldenMotes()
{
    std::minstd_rand random(Seed);
    std::uniform_real_distribution<float> column(ColumnMargin, 1.0f - ColumnMargin);
    std::uniform_real_distribution<float> speed(SlowestRisesPerSecond, FastestRisesPerSecond);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    std::uniform_real_distribution<float> sway(0.0f, MaximumSway);
    std::uniform_real_distribution<float> radius(SmallestRadius, BiggestRadius);
    for (auto& mote : m_motes)
    {
        mote.Column = column(random);
        mote.RisesPerSecond = speed(random);
        mote.Phase = unit(random);
        mote.Sway = sway(random);
        mote.Radius = radius(random);
    }
}

void GoldenMotes::Render(float x, float y, float size, float seconds, float opacity) const
{
    for (const auto& mote : m_motes)
    {
        RenderMote(mote, x, y, size, seconds, opacity);
    }
}

void GoldenMotes::RenderMote(const Mote& mote, float x, float y, float size, float seconds, float opacity) const
{
    const float rise = std::fmod(seconds * mote.RisesPerSecond + mote.Phase, 1.0f);

    // It fades in at the bottom and out at the top.
    const float brightness = std::sin(rise * Pi) * opacity;
    const float sway = mote.Sway * std::sin(rise * SwaysPerRise * 2 * Pi);
    const float centerX = x + size * (mote.Column + sway);
    const float centerY = y + size * (1.0f - rise);

    const unsigned int center = WithAlpha(CenterColor, CenterAlpha * brightness);
    DrawCircle(centerX, centerY, mote.Radius * GlowScale, WithAlpha(GlowColor, GlowAlpha * brightness), GlowColor);
    DrawCircle(centerX, centerY, mote.Radius, center, center);
}
} // namespace UI::Quests
