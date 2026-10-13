#include "GameLogic/Monsters/GroundMarker.h"

namespace GameLogic::Monsters
{
namespace
{
// Offsets of the GroundMarker message (C2 header with sub code).
constexpr int32_t PacketSize = 12;
constexpr int32_t KindOffset = 5;
constexpr int32_t XOffset = 6;
constexpr int32_t YOffset = 7;
constexpr int32_t RadiusOffset = 8;
constexpr int32_t DurationOffset = 10;

// The duration is little endian.
int ReadDuration(const BYTE* data)
{
    return data[DurationOffset] | (data[DurationOffset + 1] << 8);
}
} // namespace

std::optional<GroundMarker> ReadGroundMarker(const BYTE* data, int32_t size)
{
    if (data == nullptr || size < PacketSize)
    {
        return std::nullopt;
    }

    const auto kind = static_cast<GroundMarkerKind>(data[KindOffset]);
    if (kind != GroundMarkerKind::Warning && kind != GroundMarkerKind::Hit)
    {
        return std::nullopt;
    }

    GroundMarker marker;
    marker.Kind = kind;
    marker.X = data[XOffset];
    marker.Y = data[YOffset];
    marker.Radius = data[RadiusOffset];
    marker.DurationMilliseconds = kind == GroundMarkerKind::Warning ? ReadDuration(data) : 0;
    return marker;
}
} // namespace GameLogic::Monsters
