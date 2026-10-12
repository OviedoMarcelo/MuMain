#include "Core/Platform/WinCompat.h"
#include "GameLogic/Monsters/GroundMarker.h"
#include "GameLogic/Monsters/GroundMarkerList.h"

#include "doctest.h"

#include <vector>

namespace
{
using GameLogic::Monsters::GroundMarkerKind;
using GameLogic::Monsters::GroundMarkers;
using GameLogic::Monsters::ReadGroundMarker;

constexpr size_t PacketSize = 12;
constexpr size_t KindOffset = 5;
constexpr size_t XOffset = 6;
constexpr size_t YOffset = 7;
constexpr size_t RadiusOffset = 8;
constexpr size_t DurationOffset = 10;

// A GroundMarker message: kind, x, y, radius, little endian duration.
std::vector<BYTE> MakePacket(BYTE kind, BYTE x, BYTE y, BYTE radius, WORD duration)
{
    std::vector<BYTE> packet(PacketSize);
    packet[KindOffset] = kind;
    packet[XOffset] = x;
    packet[YOffset] = y;
    packet[RadiusOffset] = radius;
    packet[DurationOffset] = static_cast<BYTE>(duration & 0xFF);
    packet[DurationOffset + 1] = static_cast<BYTE>(duration >> 8);
    return packet;
}

auto Read(const std::vector<BYTE>& packet)
{
    return ReadGroundMarker(packet.data(), static_cast<int32_t>(packet.size()));
}
} // namespace

TEST_CASE("ground marker reads a warning")
{
    const auto marker = Read(MakePacket(0, 100, 120, 3, 3000));

    REQUIRE(marker.has_value());
    CHECK(marker->Kind == GroundMarkerKind::Warning);
    CHECK(marker->X == 100);
    CHECK(marker->Y == 120);
    CHECK(marker->Radius == 3);
    CHECK(marker->DurationMilliseconds == 3000);
}

TEST_CASE("ground marker ignores an unknown kind and a truncated message")
{
    CHECK_FALSE(Read(MakePacket(5, 1, 1, 1, 100)).has_value());

    auto packet = MakePacket(0, 1, 1, 1, 100);
    packet.pop_back();
    CHECK_FALSE(Read(packet).has_value());
}

TEST_CASE("ground marker list forgets a warning after its duration")
{
    GroundMarkers().Reset();
    GroundMarkers().Add(*Read(MakePacket(0, 10, 10, 2, 3000)), 1000);

    GroundMarkers().RemoveExpired(3999);
    CHECK(GroundMarkers().Markers().size() == 1);

    GroundMarkers().RemoveExpired(4000);
    CHECK(GroundMarkers().Markers().empty());
}

TEST_CASE("ground marker hit replaces the warning at its place")
{
    GroundMarkers().Reset();
    GroundMarkers().Add(*Read(MakePacket(0, 10, 10, 2, 3000)), 1000);
    GroundMarkers().Add(*Read(MakePacket(0, 20, 20, 2, 3000)), 1000);
    GroundMarkers().Add(*Read(MakePacket(1, 10, 10, 2, 0)), 3900);

    REQUIRE(GroundMarkers().Markers().size() == 2);
    CHECK(GroundMarkers().Markers()[0].Marker.X == 20);
    CHECK(GroundMarkers().Markers()[1].Marker.Kind == GroundMarkerKind::Hit);
}

TEST_CASE("ground marker list keeps working when the millisecond counter wraps around")
{
    GroundMarkers().Reset();
    GroundMarkers().Add(*Read(MakePacket(0, 10, 10, 2, 3000)), 0xFFFFFF00u);

    GroundMarkers().RemoveExpired(0x00000010u);
    CHECK(GroundMarkers().Markers().size() == 1);
}
