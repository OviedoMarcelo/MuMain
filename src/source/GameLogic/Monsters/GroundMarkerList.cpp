#include "GameLogic/Monsters/GroundMarkerList.h"

#include <algorithm>

namespace GameLogic::Monsters
{
namespace
{
// A hit is shown shortly, after the warning ended.
constexpr std::uint32_t HitMilliseconds = 600;

// More markers than these are a misbehaving server; the oldest ones go.
constexpr size_t MaximumMarkers = 64;
} // namespace

GroundMarkerList& GroundMarkerList::Instance()
{
    static GroundMarkerList instance;
    return instance;
}

void GroundMarkerList::Reset()
{
    this->m_markers.clear();
}

void GroundMarkerList::Add(const GroundMarker& marker, std::uint32_t nowMilliseconds)
{
    const std::uint32_t duration = marker.Kind == GroundMarkerKind::Hit
                                       ? HitMilliseconds
                                       : static_cast<std::uint32_t>(marker.DurationMilliseconds);
    if (duration == 0)
    {
        return;
    }

    // The hit replaces the warning at its place, even when the clocks of the server and the client differ a bit.
    if (marker.Kind == GroundMarkerKind::Hit)
    {
        std::erase_if(this->m_markers, [&marker](const ActiveGroundMarker& active) {
            return active.Marker.Kind == GroundMarkerKind::Warning && active.Marker.X == marker.X && active.Marker.Y == marker.Y;
        });
    }

    if (this->m_markers.size() >= MaximumMarkers)
    {
        this->m_markers.erase(this->m_markers.begin());
    }

    this->m_markers.push_back({marker, nowMilliseconds, nowMilliseconds + duration});
}

void GroundMarkerList::RemoveExpired(std::uint32_t nowMilliseconds)
{
    // The difference stays right when the millisecond counter wraps around.
    std::erase_if(this->m_markers, [nowMilliseconds](const ActiveGroundMarker& active) {
        return static_cast<std::int32_t>(nowMilliseconds - active.EndMilliseconds) >= 0;
    });
}

const std::vector<ActiveGroundMarker>& GroundMarkerList::Markers() const
{
    return this->m_markers;
}
} // namespace GameLogic::Monsters
