#include "Core/Platform/WinCompat.h"
#include "Data/Translation/MultiLanguage.h"
#include "GameLogic/Monsters/BossHealth.h"
#include "GameLogic/Monsters/BossHealthState.h"

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
using GameLogic::Monsters::BossHealthBar;
using GameLogic::Monsters::ReadBossHealth;

constexpr size_t PacketSize = 76;
constexpr size_t StateOffset = 5;
constexpr size_t ObjectIdOffset = 6;
constexpr size_t HealthOffset = 8;
constexpr size_t BossNameOffset = 12;
constexpr size_t PhaseNameOffset = 44;

// A BossHealth message: state, big endian object id, health, UTF-8 names.
std::vector<BYTE> MakePacket(BYTE state, WORD objectId, BYTE health, const std::string& bossName, const std::string& phaseName)
{
    std::vector<BYTE> packet(PacketSize);
    packet[StateOffset] = state;
    packet[ObjectIdOffset] = static_cast<BYTE>(objectId >> 8);
    packet[ObjectIdOffset + 1] = static_cast<BYTE>(objectId & 0xFF);
    packet[HealthOffset] = health;
    std::memcpy(packet.data() + BossNameOffset, bossName.data(), bossName.size());
    std::memcpy(packet.data() + PhaseNameOffset, phaseName.data(), phaseName.size());
    return packet;
}

auto Read(const std::vector<BYTE>& packet)
{
    return ReadBossHealth(packet.data(), static_cast<int32_t>(packet.size()));
}
} // namespace

TEST_CASE("boss health reads the names and the health")
{
    const auto health = Read(MakePacket(1, 0x8000 | 0x0123, 42, "Kundun Reborn", "Escudo"));

    REQUIRE(health.has_value());
    CHECK(health->IsVisible);
    CHECK(health->Key == 0x0123);
    CHECK(health->HealthPercent == 42);
    CHECK(health->BossName == L"Kundun Reborn");
    CHECK(health->PhaseName == L"Escudo");
}

TEST_CASE("boss health limits the health to 100 percent")
{
    const auto health = Read(MakePacket(1, 7, 250, "Boss", ""));

    REQUIRE(health.has_value());
    CHECK(health->HealthPercent == 100);
}

TEST_CASE("boss health ignores a truncated message")
{
    auto packet = MakePacket(1, 7, 50, "Boss", "");
    packet.pop_back();

    CHECK_FALSE(Read(packet).has_value());
}

TEST_CASE("boss health bar is hidden only for its own boss")
{
    BossHealthBar().Reset();
    BossHealthBar().Apply(*Read(MakePacket(1, 7, 80, "Boss", "")));
    REQUIRE(BossHealthBar().Current() != nullptr);

    BossHealthBar().Apply(*Read(MakePacket(0, 8, 0, "", "")));
    CHECK(BossHealthBar().Current() != nullptr);

    BossHealthBar().Apply(*Read(MakePacket(0, 7, 0, "", "")));
    CHECK(BossHealthBar().Current() == nullptr);
}
