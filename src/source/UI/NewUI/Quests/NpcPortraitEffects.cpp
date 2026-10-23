#include "UI/NewUI/Quests/NpcPortraitEffects.h"

#include <array>
#include <utility>

namespace UI::Quests::NpcPortraitEffects
{
const RisingParticleStyle GoldenMotes = {
    .Count = 8,
    .Shape = RisingParticleShape::Dot,
    .Seed = 242,
    .SlowestRisesPerSecond = 0.12f,
    .FastestRisesPerSecond = 0.25f,
    .HighestStart = 0.0f,
    .ShortestRise = 1.0f,
    .MaximumDrift = 0.0f,
    .MaximumSway = 0.06f,
    .SwaysPerRise = 1.5f,
    .SmallestRadius = 0.8f,
    .BiggestRadius = 1.6f,
    .CenterColorAtBottom = 0xFFF0C0u,
    .CenterColorAtTop = 0xFFF0C0u,
    .CenterAlpha = 0.9f,
    .GlowColorAtBottom = 0xF0C25Au,
    .GlowColorAtTop = 0xF0C25Au,
    .GlowAlpha = 0.45f,
    .GlowScale = 3.0f,
};

// Like the sparks of a campfire: thin, short lines which fly up in a fan, from bright orange
// to red, slow down, and fall back as little embers which go out. They flicker all the time.
const RisingParticleStyle FireSparks = {
    .Count = 24,
    .Shape = RisingParticleShape::Streak,
    .Seed = 249,
    .SlowestRisesPerSecond = 0.35f,
    .FastestRisesPerSecond = 0.7f,
    .HighestStart = 0.35f,
    .ShortestRise = 0.35f,
    .MaximumDrift = 0.35f,
    .SmallestRadius = 0.2f,
    .BiggestRadius = 0.3f,
    .ShortestStreak = 3.5f,
    .LongestStreak = 8.0f,
    .CenterColorAtBottom = 0xFFB347u,
    .CenterColorAtTop = 0xC8280Au,
    .CenterAlpha = 1.0f,
    .FallShare = 0.4f,
    .FallDistance = 0.25f,
    .EmberScale = 1.6f,
    .FlickersPerSecond = 7.0f,
    .FlickerDepth = 0.6f,
};

// Little lights of blue magic: they float up slowly, waving, from light blue to violet, and twinkle.
const RisingParticleStyle ArcaneSparkles = {
    .Count = 12,
    .Shape = RisingParticleShape::Dot,
    .Seed = 417,
    .SlowestRisesPerSecond = 0.15f,
    .FastestRisesPerSecond = 0.3f,
    .HighestStart = 0.4f,
    .ShortestRise = 0.5f,
    .MaximumDrift = 0.05f,
    .MaximumSway = 0.08f,
    .SwaysPerRise = 2.0f,
    .SmallestRadius = 0.6f,
    .BiggestRadius = 1.2f,
    .CenterColorAtBottom = 0xE0F4FFu,
    .CenterColorAtTop = 0xC8D8FFu,
    .CenterAlpha = 0.95f,
    .GlowColorAtBottom = 0x3A8CFFu,
    .GlowColorAtTop = 0x7A5CFFu,
    .GlowAlpha = 0.5f,
    .GlowScale = 3.0f,
    .FlickersPerSecond = 3.0f,
    .FlickerDepth = 0.7f,
};

namespace
{
// The numbers are the ones of the NPC definitions, like the names of the portraits.
constexpr uint16_t ElfLala = 242;
constexpr uint16_t BerdyshGuard = 249;
constexpr uint16_t CrossbowGuard = 247;
constexpr uint16_t Guard = 220;
constexpr uint16_t Marce = 417;

const std::array<std::pair<uint16_t, const RisingParticleStyle*>, 5> EffectsByNpc = {{
    {ElfLala, &GoldenMotes},
    {BerdyshGuard, &FireSparks},
    {CrossbowGuard, &FireSparks},
    {Guard, &FireSparks},
    {Marce, &ArcaneSparkles},
}};
} // namespace

const RisingParticleStyle* ForNpc(uint16_t npcNumber)
{
    for (const auto& [number, style] : EffectsByNpc)
    {
        if (number == npcNumber)
        {
            return style;
        }
    }

    return nullptr;
}
} // namespace UI::Quests::NpcPortraitEffects
