#include "Core/Platform/WinCompat.h"
#include "GameLogic/Monsters/ServerMonsterLevelCatalog.h"

#include "doctest.h"

#include <vector>

namespace
{
using GameLogic::Monsters::GetLevelDifficulty;
using GameLogic::Monsters::LevelDifficulty;
using GameLogic::Monsters::ServerMonsterLevels;

constexpr size_t CountOffset = 6;
constexpr size_t EntriesOffset = 8;
constexpr size_t EntrySize = 4;

void WriteWord(std::vector<BYTE>& packet, size_t offset, WORD value)
{
    packet[offset] = static_cast<BYTE>(value & 0xFF);
    packet[offset + 1] = static_cast<BYTE>(value >> 8);
}

std::vector<BYTE> MakePacket(size_t count)
{
    std::vector<BYTE> packet(EntriesOffset + count * EntrySize);
    WriteWord(packet, CountOffset, static_cast<WORD>(count));
    return packet;
}

// A MonsterLevel entry: monster number, level.
void WriteMonster(std::vector<BYTE>& packet, size_t index, WORD number, WORD level)
{
    const auto offset = EntriesOffset + index * EntrySize;
    WriteWord(packet, offset + 0, number);
    WriteWord(packet, offset + 2, level);
}
} // namespace

TEST_CASE("server monster level catalog reads the levels")
{
    ServerMonsterLevels().Reset();
    auto packet = MakePacket(2);
    WriteMonster(packet, 0, 0, 4);
    WriteMonster(packet, 1, 300, 120);

    REQUIRE(ServerMonsterLevels().AddFromPacket(packet.data(), static_cast<int32_t>(packet.size())));

    CHECK(ServerMonsterLevels().Find(0) == 4);
    CHECK(ServerMonsterLevels().Find(300) == 120);
    CHECK(ServerMonsterLevels().Find(1) == -1);
}

TEST_CASE("server monster level catalog ignores a truncated message")
{
    ServerMonsterLevels().Reset();
    auto packet = MakePacket(2);
    WriteMonster(packet, 0, 7, 34);
    packet.pop_back();

    CHECK_FALSE(ServerMonsterLevels().AddFromPacket(packet.data(), static_cast<int32_t>(packet.size())));
    CHECK(ServerMonsterLevels().Find(7) == -1);
}

TEST_CASE("server monster level catalog forgets the levels on reset")
{
    auto packet = MakePacket(1);
    WriteMonster(packet, 0, 7, 34);
    REQUIRE(ServerMonsterLevels().AddFromPacket(packet.data(), static_cast<int32_t>(packet.size())));

    ServerMonsterLevels().Reset();

    CHECK(ServerMonsterLevels().Find(7) == -1);
}

TEST_CASE("monster level difficulty depends on the level difference")
{
    CHECK(GetLevelDifficulty(40, 50) == LevelDifficulty::Trivial);
    CHECK(GetLevelDifficulty(41, 50) == LevelDifficulty::Even);
    CHECK(GetLevelDifficulty(54, 50) == LevelDifficulty::Even);
    CHECK(GetLevelDifficulty(55, 50) == LevelDifficulty::Hard);
    CHECK(GetLevelDifficulty(59, 50) == LevelDifficulty::Hard);
    CHECK(GetLevelDifficulty(60, 50) == LevelDifficulty::Deadly);
}
