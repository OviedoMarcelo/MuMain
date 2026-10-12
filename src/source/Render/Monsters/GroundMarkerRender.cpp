#include "stdafx.h"

#include "Render/Monsters/GroundMarkerRender.h"

#include "Audio/DSPlaySound.h"
#include "GameLogic/Events/Event.h"
#include "GameLogic/Monsters/GroundMarkerList.h"
#include "Render/Effects/ZzzEffect.h"
#include "Render/Terrain/ZzzLodTerrain.h"
#include "Render/Textures/ZzzOpenglUtil.h"

#include <algorithm>
#include <cmath>

namespace Render::Monsters::GroundMarkers
{
namespace
{
using GameLogic::Monsters::ActiveGroundMarker;
using GameLogic::Monsters::GroundMarker;
using GameLogic::Monsters::GroundMarkerKind;

// The magic circle on the ground, which skills like the one of the Archangel use, too.
constexpr int CircleTexture = BITMAP_MAGIC + 1;

constexpr float HalfTile = 0.5f;
constexpr float PulseSpeed = 0.012f;
constexpr float PulseBase = 0.7f;
constexpr float PulseAmplitude = 0.3f;
constexpr float RotationSpeed = 0.05f;

// The warning starts dim and is fully bright right before the hit.
constexpr float WarningStartBrightness = 0.35f;
constexpr float WarningGreen = 0.12f;
constexpr float WarningBlue = 0.08f;

// The flash of the hit gets a bit bigger while it fades.
constexpr float HitGrowth = 0.4f;
constexpr float HitGreen = 0.55f;
constexpr float HitBlue = 0.15f;

float ToWorld(int tile)
{
    return (static_cast<float>(tile) + HalfTile) * TERRAIN_SCALE;
}

float GetDiameter(const GroundMarker& marker)
{
    return static_cast<float>(marker.Radius * 2 + 1);
}

float GetProgress(const ActiveGroundMarker& active, std::uint32_t now)
{
    const auto duration = static_cast<float>(active.EndMilliseconds - active.StartMilliseconds);
    const auto elapsed = static_cast<float>(now - active.StartMilliseconds);
    return duration > 0.f ? std::clamp(elapsed / duration, 0.f, 1.f) : 1.f;
}

void RenderWarning(const ActiveGroundMarker& active, std::uint32_t now)
{
    const auto& marker = active.Marker;
    const float progress = GetProgress(active, now);
    const float pulse = PulseBase + PulseAmplitude * std::sin(static_cast<float>(now) * PulseSpeed);
    const float brightness = (WarningStartBrightness + (1.f - WarningStartBrightness) * progress) * pulse;
    vec3_t light;
    Vector(brightness, brightness * WarningGreen, brightness * WarningBlue, light);

    const float diameter = GetDiameter(marker);
    const float rotation = static_cast<float>(now) * RotationSpeed;
    RenderTerrainAlphaBitmap(CircleTexture, ToWorld(marker.X), ToWorld(marker.Y), diameter, diameter, light, rotation);
}

void RenderHit(const ActiveGroundMarker& active, std::uint32_t now)
{
    const auto& marker = active.Marker;
    const float progress = GetProgress(active, now);
    const float brightness = 1.f - progress;
    vec3_t light;
    Vector(brightness, brightness * HitGreen, brightness * HitBlue, light);

    const float diameter = GetDiameter(marker) * (1.f + HitGrowth * progress);
    RenderTerrainAlphaBitmap(CircleTexture, ToWorld(marker.X), ToWorld(marker.Y), diameter, diameter, light);
}
} // namespace

void Render()
{
    auto& markers = GameLogic::Monsters::GroundMarkers();
    const std::uint32_t now = GetMillisecondsTimestamp();
    markers.RemoveExpired(now);
    if (markers.Markers().empty())
    {
        return;
    }

    EnableAlphaBlend();
    for (const auto& active : markers.Markers())
    {
        if (active.Marker.Kind == GroundMarkerKind::Hit)
        {
            RenderHit(active, now);
        }
        else
        {
            RenderWarning(active, now);
        }
    }
}

void ShowHit(const GroundMarker& marker)
{
    vec3_t position;
    vec3_t angle;
    vec3_t light;
    const float x = ToWorld(marker.X);
    const float y = ToWorld(marker.Y);
    Vector(x, y, RequestTerrainHeight(x, y), position);
    Vector(0.f, 0.f, 0.f, angle);
    Vector(1.f, 1.f, 1.f, light);
    CreateEffect(MODEL_FIRE, position, angle, light);
    PlayBuffer(SOUND_METEORITE01);
}
} // namespace Render::Monsters::GroundMarkers
