#pragma once

#include <vector>

namespace UI::Quests
{
enum class RisingParticleShape
{
    // A round glow around a bright center.
    Dot,
    // A thin line along the way it moves, bright at its head and fading out to its tail, like a spark.
    Streak,
};

// How rising particles look and move, e.g. golden motes or fire sparks.
// The colors are 0x00RRGGBB; the alphas are separate, from 0 to 1.
// The distances are relative to the size of the area, unless they say otherwise.
struct RisingParticleStyle
{
    int Count = 0;
    RisingParticleShape Shape = RisingParticleShape::Dot;
    // The same seed gives the same particles every time.
    unsigned int Seed = 0;
    // How many times per second a particle goes its way, from where it starts to where it fades out.
    float SlowestRisesPerSecond = 0.0f;
    float FastestRisesPerSecond = 0.0f;
    // A particle starts this high at most, so that not all of them start at the bottom.
    float HighestStart = 0.0f;
    // The shortest way up which a particle goes before it fades out; the longest one reaches the top.
    float ShortestRise = 1.0f;
    // How far a particle moves to a side on its way up, at most, in a straight line.
    float MaximumDrift = 0.0f;
    // How far a particle sways to the sides, at most, and how often on its way up.
    float MaximumSway = 0.0f;
    float SwaysPerRise = 0.0f;
    // The radius of a dot, or half the width of a streak, in the units of the area.
    float SmallestRadius = 0.0f;
    float BiggestRadius = 0.0f;
    // The length of a streak, in the units of the area.
    float ShortestStreak = 0.0f;
    float LongestStreak = 0.0f;
    // The color of the particle changes from the first one to the second one on its way up.
    unsigned int CenterColorAtBottom = 0;
    unsigned int CenterColorAtTop = 0;
    float CenterAlpha = 0.0f;
    // The glow around a particle, which changes its color on its way up, too. An alpha of 0 leaves it out.
    unsigned int GlowColorAtBottom = 0;
    unsigned int GlowColorAtTop = 0;
    float GlowAlpha = 0.0f;
    // How much bigger (for a dot) or wider (for a streak) the glow is than the particle.
    float GlowScale = 0.0f;
    // The share of its time (0 to 1) which a particle spends falling down after it was thrown up, e.g. a spark:
    // it slows down on its way up and falls as a dot (an ember) of EmberScale times its radius, for FallDistance.
    // With 0, it rises steadily the whole time instead.
    float FallShare = 0.0f;
    float FallDistance = 0.0f;
    float EmberScale = 1.0f;
    // How often a particle flickers per second, and how much of its brightness it loses then (0 to 1).
    float FlickersPerSecond = 0.0f;
    float FlickerDepth = 0.0f;
};

// Particles which rise over a square area, e.g. the portrait of an NPC, and
// fade in and out on their way. Their place only depends on the time, so
// there's nothing to update between the frames.
class RisingParticles
{
public:
    explicit RisingParticles(const RisingParticleStyle& style);

    // Draws the particles over the square at x, y, after the seconds since they started.
    // The opacity (0 to 1) fades all of them, e.g. while their window opens.
    void Render(float x, float y, float size, float seconds, float opacity) const;

private:
    struct Particle
    {
        // Where it starts, from 0 (left) to 1 (right) and from 0 (bottom) up.
        float Column = 0.0f;
        float Start = 0.0f;
        // How far it goes up and to the side.
        float Rise = 0.0f;
        float Drift = 0.0f;
        float RisesPerSecond = 0.0f;
        // Where on its way it is at first, from 0 to 1, so that they don't go together.
        float Phase = 0.0f;
        float Sway = 0.0f;
        float Radius = 0.0f;
        float StreakLength = 0.0f;
    };

    // Where a particle is, in the units of the area, and the way it moves.
    struct Placement
    {
        float X = 0.0f;
        float Y = 0.0f;
        float DirectionX = 0.0f;
        float DirectionY = 0.0f;
    };

    Placement PlaceAt(const Particle& particle, float height, float sideways, float x, float y, float size) const;
    void RenderParticle(const Particle& particle, float x, float y, float size, float seconds, float opacity) const;
    void RenderSteadyRise(const Particle& particle, float progress, float brightness, float x, float y,
                          float size) const;
    void RenderThrowUp(const Particle& particle, float progress, float brightness, float x, float y, float size) const;
    void RenderFallDown(const Particle& particle, float progress, float brightness, float x, float y,
                        float size) const;
    // The progress (0 to 1) chooses the color between the one at the bottom and the one at the top.
    void RenderDot(const Placement& place, float radius, float progress, float brightness) const;
    void RenderStreak(const Placement& place, float radius, float length, float progress, float brightness) const;
    float GetFlicker(const Particle& particle, float seconds) const;

    RisingParticleStyle m_style;
    std::vector<Particle> m_particles;
};
} // namespace UI::Quests
