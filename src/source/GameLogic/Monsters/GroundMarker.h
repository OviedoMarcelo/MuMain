#pragma once

#include "Core/Platform/WinCompat.h"

#include <cstdint>
#include <optional>

// Where an attack of a boss hits after a delay, e.g. a meteor.
//
// The server sends a GroundMarker message with the kind "warning" when the
// boss marks the place, and with the kind "hit" when the attack hits. Servers
// which don't know the message never send it, so no marker is shown.
namespace GameLogic::Monsters
{
enum class GroundMarkerKind : BYTE
{
    Warning = 0,
    Hit = 1,
};

struct GroundMarker
{
    GroundMarkerKind Kind = GroundMarkerKind::Warning;
    int X = 0;
    int Y = 0;

    // In tiles.
    int Radius = 0;

    // How long the warning is shown; 0 for a hit.
    int DurationMilliseconds = 0;
};

// Reads the GroundMarker message, or nothing when the data isn't plausible, so
// that the caller can ignore it.
std::optional<GroundMarker> ReadGroundMarker(const BYTE* data, int32_t size);
} // namespace GameLogic::Monsters
