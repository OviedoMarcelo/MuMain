#include "Core/Platform/WinCompat.h"
#include "Data/Translation/MultiLanguage.h"
#include "GameLogic/Quests/SeasonPassCatalog.h"

#include "doctest.h"

#include <cstring>
#include <string>
#include <vector>

// The texts of these tests are ASCII, so the client's UTF-8 conversion isn't needed.
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
using GameLogic::Quests::SeasonPass;

constexpr size_t HeaderSize = 56;
constexpr size_t LevelSize = 132;

void WriteWord(std::vector<BYTE>& packet, size_t offset, WORD value)
{
    packet[offset] = static_cast<BYTE>(value & 0xFF);
    packet[offset + 1] = static_cast<BYTE>(value >> 8);
}

void WriteUInt32(std::vector<BYTE>& packet, size_t offset, uint32_t value)
{
    for (size_t i = 0; i < sizeof(value); ++i)
    {
        packet[offset + i] = static_cast<BYTE>((value >> (i * 8)) & 0xFF);
    }
}

void WriteText(std::vector<BYTE>& packet, size_t offset, const std::string& text)
{
    std::memcpy(packet.data() + offset, text.data(), text.size());
}

// A SeasonPassState message: premium flag, level, maximum level, level count, experience, end, name.
std::vector<BYTE> MakePacket(bool isPremium, WORD level, WORD maximumLevel, size_t levelCount, const std::string& name)
{
    std::vector<BYTE> packet(HeaderSize + levelCount * LevelSize);
    packet[5] = isPremium ? 1 : 0;
    WriteWord(packet, 6, level);
    WriteWord(packet, 8, maximumLevel);
    WriteWord(packet, 10, static_cast<WORD>(levelCount));
    WriteUInt32(packet, 12, 250);
    WriteUInt32(packet, 16, 1000);
    WriteUInt32(packet, 20, 3600);
    WriteText(packet, 24, name);
    return packet;
}

void WriteLevel(std::vector<BYTE>& packet, size_t index, WORD level, bool freeClaimed, bool premiumClaimed,
                const std::string& freeRewards, const std::string& premiumRewards)
{
    const auto offset = HeaderSize + index * LevelSize;
    WriteWord(packet, offset, level);
    packet[offset + 2] = freeClaimed ? 1 : 0;
    packet[offset + 3] = premiumClaimed ? 1 : 0;
    WriteText(packet, offset + 4, freeRewards);
    WriteText(packet, offset + 68, premiumRewards);
}

bool Add(const std::vector<BYTE>& packet)
{
    return SeasonPass().AddFromPacket(packet.data(), static_cast<int32_t>(packet.size()));
}
} // namespace

TEST_CASE("season pass catalog reads the season and its levels")
{
    SeasonPass().Reset();
    auto packet = MakePacket(false, 1, 10, 2, "Temporada 1");
    WriteLevel(packet, 0, 1, true, false, "1000000 Zen", "2000000 EXP");
    WriteLevel(packet, 1, 2, false, false, "2000000 Zen", "");

    REQUIRE(Add(packet));

    CHECK(SeasonPass().IsAvailable());
    CHECK(SeasonPass().HasSeason());
    CHECK(SeasonPass().GetSeasonName() == L"Temporada 1");
    CHECK(SeasonPass().GetLevel() == 1);
    CHECK(SeasonPass().GetMaximumLevel() == 10);
    CHECK(SeasonPass().GetExperienceInLevel() == 250);
    CHECK(SeasonPass().GetExperiencePerLevel() == 1000);
    REQUIRE(SeasonPass().GetLevels().size() == 2);
    CHECK(SeasonPass().GetLevels()[0].FreeRewards == L"1000000 Zen");
    CHECK(SeasonPass().GetLevels()[0].IsFreeClaimed);
    CHECK(SeasonPass().GetLevels()[1].PremiumRewards.empty());
}

TEST_CASE("season pass catalog knows which rewards can be claimed")
{
    SeasonPass().Reset();
    auto packet = MakePacket(false, 1, 10, 2, "Temporada 1");
    WriteLevel(packet, 0, 1, true, false, "1000000 Zen", "2000000 EXP");
    WriteLevel(packet, 1, 2, false, false, "2000000 Zen", "");
    REQUIRE(Add(packet));

    // The free reward of level 1 was received, premium needs the pass and level 2 isn't reached.
    CHECK_FALSE(SeasonPass().HasClaimableRewards());

    packet[5] = 1;
    REQUIRE(Add(packet));
    CHECK(SeasonPass().IsClaimable(SeasonPass().GetLevels()[0]));
    CHECK_FALSE(SeasonPass().IsClaimable(SeasonPass().GetLevels()[1]));
}

TEST_CASE("season pass catalog knows when no season is running")
{
    SeasonPass().Reset();
    REQUIRE(Add(MakePacket(false, 0, 0, 0, "")));

    CHECK(SeasonPass().IsAvailable());
    CHECK_FALSE(SeasonPass().HasSeason());
}

TEST_CASE("season pass catalog ignores a truncated message")
{
    SeasonPass().Reset();
    auto packet = MakePacket(false, 1, 10, 1, "Temporada 1");
    packet.pop_back();

    CHECK_FALSE(Add(packet));
    CHECK_FALSE(SeasonPass().IsAvailable());
}
