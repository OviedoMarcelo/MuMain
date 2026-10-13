#include "Core/Platform/WinCompat.h"
#include "Data/Translation/MultiLanguage.h"
#include "GameLogic/Monsters/MonsterAppearance.h"

#include "doctest.h"

#include <cstring>
#include <string>
#include <vector>

// The names of these tests are ASCII, so the client's UTF-8 conversion isn't needed.
int32_t CMultiLanguage::ConvertFromUtf8(wchar_t* target, const char* source, int maxSourceLength)
{
    int32_t length = 0;
    while (length < maxSourceLength && source[length] != '\0')
    {
        target[length] = static_cast<unsigned char>(source[length]);
        ++length;
    }

    target[length] = L'\0';
    return length;
}

namespace
{
using GameLogic::Monsters::MonsterGlow;
using GameLogic::Monsters::ReadMonsterAppearance;

constexpr size_t PacketSize = 44;
constexpr size_t GlowOffset = 5;
constexpr size_t ObjectIdOffset = 6;
constexpr size_t ScaleOffset = 8;
constexpr size_t NameOffset = 12;

// A MonsterAppearance message: glow, big endian object id, little endian scale, UTF-8 name.
std::vector<BYTE> MakePacket(WORD objectId, WORD scalePercent, BYTE glow, const std::string& name)
{
    std::vector<BYTE> packet(PacketSize);
    packet[GlowOffset] = glow;
    packet[ObjectIdOffset] = static_cast<BYTE>(objectId >> 8);
    packet[ObjectIdOffset + 1] = static_cast<BYTE>(objectId & 0xFF);
    packet[ScaleOffset] = static_cast<BYTE>(scalePercent & 0xFF);
    packet[ScaleOffset + 1] = static_cast<BYTE>(scalePercent >> 8);
    std::memcpy(packet.data() + NameOffset, name.data(), name.size());
    return packet;
}

auto Read(const std::vector<BYTE>& packet)
{
    return ReadMonsterAppearance(packet.data(), static_cast<int32_t>(packet.size()));
}
} // namespace

TEST_CASE("monster appearance reads the size, the glow and the name")
{
    const auto appearance = Read(MakePacket(0x1234, 300, 15, "Kundun Reborn"));

    REQUIRE(appearance.has_value());
    CHECK(appearance->Key == 0x1234);
    CHECK(appearance->ScalePercent == 300);
    CHECK(appearance->Glow == MonsterGlow::Level15);
    CHECK(appearance->Name == L"Kundun Reborn");
}

TEST_CASE("monster appearance ignores the flag of a monster which appears new")
{
    const auto appearance = Read(MakePacket(0x8000 | 0x0042, 100, 100, ""));

    REQUIRE(appearance.has_value());
    CHECK(appearance->Key == 0x0042);
    CHECK(appearance->Glow == MonsterGlow::Golden);
    CHECK(appearance->Name.empty());
}

TEST_CASE("monster appearance shows an unknown glow as no glow")
{
    const auto appearance = Read(MakePacket(7, 100, 8, ""));

    REQUIRE(appearance.has_value());
    CHECK(appearance->Glow == MonsterGlow::None);
}

TEST_CASE("monster appearance ignores a size of zero")
{
    CHECK_FALSE(Read(MakePacket(7, 0, 0, "")).has_value());
}

TEST_CASE("monster appearance ignores a truncated message")
{
    auto packet = MakePacket(3, 150, 7, "Boss");
    packet.pop_back();

    CHECK_FALSE(Read(packet).has_value());
}
