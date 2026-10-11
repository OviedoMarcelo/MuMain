#pragma once

#include "GameLogic/Monsters/GroundMarker.h"

#include <cstdint>
#include <vector>

namespace GameLogic::Monsters
{
struct ActiveGroundMarker
{
    GroundMarker Marker;
    std::uint32_t StartMilliseconds = 0;
    std::uint32_t EndMilliseconds = 0;
};

// The markers on the ground which are shown now (see GroundMarker). A warning
// is shown for its duration, a hit for a short flash.
class GroundMarkerList
{
public:
    static GroundMarkerList& Instance();

    // Forgets all markers, e.g. when we connect to another server.
    void Reset();

    void Add(const GroundMarker& marker, std::uint32_t nowMilliseconds);

    // Forgets the markers whose time is over.
    void RemoveExpired(std::uint32_t nowMilliseconds);

    const std::vector<ActiveGroundMarker>& Markers() const;

private:
    std::vector<ActiveGroundMarker> m_markers;
};

inline GroundMarkerList& GroundMarkers()
{
    return GroundMarkerList::Instance();
}
} // namespace GameLogic::Monsters
