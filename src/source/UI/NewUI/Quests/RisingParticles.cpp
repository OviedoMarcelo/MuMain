#include "stdafx.h"

#include "UI/NewUI/Quests/RisingParticles.h"

#include "Render/Textures/ZzzOpenglUtil.h"
#include "UI/NewUI/Quests/ColorMath.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace UI::Quests
{
namespace
{
using ColorMath::Blend;
using ColorMath::WithAlpha;

constexpr float Pi = 3.14159265f;
// The particles start away from the borders of the area.
constexpr float ColumnMargin = 0.08f;
// The circles are drawn as this many triangles around their center.
constexpr int CircleSegments = 12;

// A thrown particle fades in during this share of its way up.
constexpr float ThrowFadeInShare = 0.15f;
// A falling ember keeps drifting to its side by this share of its drift.
constexpr float FallDriftShare = 0.4f;

using Range = std::uniform_real_distribution<float>;

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

// A line of the same width from the head back along the direction, with a color for each end.
void DrawFadingLine(float headX, float headY, float directionX, float directionY, float length, float width,
                    unsigned int headColor, unsigned int tailColor)
{
    const float tailX = headX - directionX * length;
    const float tailY = headY - directionY * length;
    // Half the width across the line: the direction turned by a quarter.
    const float acrossX = -directionY * width / 2;
    const float acrossY = directionX * width / 2;

    const float x[4] = {headX + acrossX, headX - acrossX, tailX - acrossX, tailX + acrossX};
    const float y[4] = {headY + acrossY, headY - acrossY, tailY - acrossY, tailY + acrossY};
    const unsigned int colors[4] = {headColor, headColor, tailColor, tailColor};
    RenderColorQuadPointsARGB(x, y, colors);
}
} // namespace

RisingParticles::RisingParticles(const RisingParticleStyle& style)
    : m_style(style)
    , m_particles(static_cast<size_t>(style.Count))
{
    std::minstd_rand random(style.Seed);
    Range column(ColumnMargin, 1.0f - ColumnMargin);
    Range start(0.0f, style.HighestStart);
    Range rise(style.ShortestRise, 1.0f);
    Range drift(-style.MaximumDrift, style.MaximumDrift);
    Range speed(style.SlowestRisesPerSecond, style.FastestRisesPerSecond);
    Range unit(0.0f, 1.0f);
    Range sway(0.0f, style.MaximumSway);
    Range radius(style.SmallestRadius, style.BiggestRadius);
    Range streak(style.ShortestStreak, style.LongestStreak);
    for (auto& particle : m_particles)
    {
        particle.Column = column(random);
        particle.Start = start(random);
        // It fades out at the top at the latest.
        particle.Rise = rise(random) * (1.0f - particle.Start);
        particle.Drift = drift(random);
        particle.RisesPerSecond = speed(random);
        particle.Phase = unit(random);
        particle.Sway = sway(random);
        particle.Radius = radius(random);
        particle.StreakLength = streak(random);
    }
}

void RisingParticles::Render(float x, float y, float size, float seconds, float opacity) const
{
    for (const auto& particle : m_particles)
    {
        RenderParticle(particle, x, y, size, seconds, opacity);
    }
}

float RisingParticles::GetFlicker(const Particle& particle, float seconds) const
{
    if (m_style.FlickersPerSecond <= 0.0f)
    {
        return 1.0f;
    }

    // Each particle flickers in its own rhythm, which its phase shifts.
    const float wave = std::sin((seconds * m_style.FlickersPerSecond + particle.Phase) * 2 * Pi);
    return 1.0f - m_style.FlickerDepth * (0.5f + 0.5f * wave);
}

RisingParticles::Placement RisingParticles::PlaceAt(const Particle& particle, float height, float sideways,
                                                    float x, float y, float size) const
{
    Placement place;
    place.X = x + size * (particle.Column + sideways);
    place.Y = y + size * (1.0f - height);

    // Up by its rise, and to the side by its drift.
    const float length = std::hypot(particle.Drift, particle.Rise);
    place.DirectionX = length > 0.0f ? particle.Drift / length : 0.0f;
    place.DirectionY = length > 0.0f ? -particle.Rise / length : -1.0f;
    return place;
}

void RisingParticles::RenderParticle(const Particle& particle, float x, float y, float size, float seconds,
                                     float opacity) const
{
    const float progress = std::fmod(seconds * particle.RisesPerSecond + particle.Phase, 1.0f);
    const float brightness = GetFlicker(particle, seconds) * opacity;
    if (m_style.FallShare <= 0.0f)
    {
        RenderSteadyRise(particle, progress, brightness, x, y, size);
        return;
    }

    const float peak = 1.0f - m_style.FallShare;
    if (progress < peak)
    {
        RenderThrowUp(particle, progress / peak, brightness, x, y, size);
    }
    else
    {
        RenderFallDown(particle, (progress - peak) / m_style.FallShare, brightness, x, y, size);
    }
}

void RisingParticles::RenderSteadyRise(const Particle& particle, float progress, float brightness, float x, float y,
                                       float size) const
{
    const float swayAngle = progress * m_style.SwaysPerRise * 2 * Pi;
    const float sideways = particle.Drift * progress + particle.Sway * std::sin(swayAngle);
    const float height = particle.Start + particle.Rise * progress;
    const Placement place = PlaceAt(particle, height, sideways, x, y, size);

    // It fades in where it starts and out where its way ends.
    const float fading = std::sin(progress * Pi) * brightness;
    if (m_style.Shape == RisingParticleShape::Dot)
    {
        RenderDot(place, particle.Radius, progress, fading);
    }
    else
    {
        RenderStreak(place, particle.Radius, particle.StreakLength, progress, fading);
    }
}

void RisingParticles::RenderThrowUp(const Particle& particle, float progress, float brightness, float x, float y,
                                    float size) const
{
    // It slows down on its way up, like something thrown, so its streak gets shorter until it stops at the top.
    const float slowness = 1.0f - progress;
    const float travelled = 1.0f - slowness * slowness;
    const Placement place =
        PlaceAt(particle, particle.Start + particle.Rise * travelled, particle.Drift * travelled, x, y, size);

    const float fadeIn = (std::min)(progress / ThrowFadeInShare, 1.0f);
    const float length = (std::max)(particle.StreakLength * slowness, particle.Radius * 2);
    RenderStreak(place, particle.Radius, length, progress, fadeIn * brightness);
}

void RisingParticles::RenderFallDown(const Particle& particle, float progress, float brightness, float x, float y,
                                     float size) const
{
    // It falls faster and faster as an ember, still drifting a bit, and goes out.
    const float height = particle.Start + particle.Rise - m_style.FallDistance * progress * progress;
    const float sideways = particle.Drift * (1.0f + FallDriftShare * progress);
    const Placement place = PlaceAt(particle, height, sideways, x, y, size);
    RenderDot(place, particle.Radius * m_style.EmberScale, 1.0f, (1.0f - progress) * brightness);
}

void RisingParticles::RenderDot(const Placement& place, float radius, float progress, float brightness) const
{
    if (m_style.GlowAlpha > 0.0f)
    {
        const unsigned int glow = Blend(m_style.GlowColorAtBottom, m_style.GlowColorAtTop, progress);
        DrawCircle(place.X, place.Y, radius * m_style.GlowScale, WithAlpha(glow, m_style.GlowAlpha * brightness),
                   WithAlpha(glow, 0.0f));
    }

    const unsigned int color = Blend(m_style.CenterColorAtBottom, m_style.CenterColorAtTop, progress);
    const unsigned int center = WithAlpha(color, m_style.CenterAlpha * brightness);
    DrawCircle(place.X, place.Y, radius, center, center);
}

void RisingParticles::RenderStreak(const Placement& place, float radius, float length, float progress,
                                   float brightness) const
{
    const float width = radius * 2;
    if (m_style.GlowAlpha > 0.0f)
    {
        const unsigned int glow = Blend(m_style.GlowColorAtBottom, m_style.GlowColorAtTop, progress);
        DrawFadingLine(place.X, place.Y, place.DirectionX, place.DirectionY, length, width * m_style.GlowScale,
                       WithAlpha(glow, m_style.GlowAlpha * brightness), WithAlpha(glow, 0.0f));
    }

    const unsigned int color = Blend(m_style.CenterColorAtBottom, m_style.CenterColorAtTop, progress);
    DrawFadingLine(place.X, place.Y, place.DirectionX, place.DirectionY, length, width,
                   WithAlpha(color, m_style.CenterAlpha * brightness), WithAlpha(color, 0.0f));
}
} // namespace UI::Quests
