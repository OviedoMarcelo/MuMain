#pragma once

namespace GameLogic::Monsters
{
struct GroundMarker;
}

// The markers on the ground where an attack of a boss hits after a delay
// (see GameLogic::Monsters::GroundMarker).
namespace Render::Monsters::GroundMarkers
{
// Draws the markers on the terrain: a red circle which pulses and gets
// brighter until the hit, and a short flash of the hit.
void Render();

// Shows the effect of a hit, like the one of the meteor skill.
void ShowHit(const GameLogic::Monsters::GroundMarker& marker);
} // namespace Render::Monsters::GroundMarkers
