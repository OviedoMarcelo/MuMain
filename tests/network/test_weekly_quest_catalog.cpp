#include "Core/Platform/WinCompat.h"
#include "Data/Translation/MultiLanguage.h"
#include "GameLogic/Quests/WeeklyQuestCatalog.h"

#include "doctest.h"

#include <algorithm>
#include <string>
#include <vector>

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
using GameLogic::Quests::WeeklyQuests;

constexpr size_t PacketSize = 520;
constexpr size_t IndexOffset = 5;
constexpr size_t CountOffset = 6;
constexpr size_t IsUpdateOffset = 7;
constexpr size_t IsCompletedOffset = 8;
constexpr size_t CurrentCountOffset = 12;
constexpr size_t RequiredCountOffset = 16;
constexpr size_t SecondsUntilResetOffset = 20;
constexpr size_t IdOffset = 24;
constexpr size_t NameOffset = 88;

void WriteUInt32(std::vector<BYTE>& packet, size_t offset, uint32_t value)
{
    for (size_t i = 0; i < 4; ++i)
    {
        packet[offset + i] = static_cast<BYTE>(value >> (8 * i));
    }
}

void WriteString(std::vector<BYTE>& packet, size_t offset, const std::string& text)
{
    std::copy(text.begin(), text.end(), packet.begin() + static_cast<std::ptrdiff_t>(offset));
}

std::vector<BYTE> MakePacket(BYTE index, BYTE count, const std::string& id, uint32_t currentCount = 0,
                             bool isUpdate = false)
{
    std::vector<BYTE> packet(PacketSize);
    packet[IndexOffset] = index;
    packet[CountOffset] = count;
    packet[IsUpdateOffset] = isUpdate ? 1 : 0;
    WriteUInt32(packet, CurrentCountOffset, currentCount);
    WriteUInt32(packet, RequiredCountOffset, 10);
    WriteUInt32(packet, SecondsUntilResetOffset, 3600);
    WriteString(packet, IdOffset, id);
    WriteString(packet, NameOffset, "Quest " + id);
    return packet;
}

bool Add(const std::vector<BYTE>& packet)
{
    return WeeklyQuests().AddFromPacket(packet.data(), static_cast<int32_t>(packet.size()));
}
} // namespace

TEST_CASE("weekly quest catalog accepts a complete ordered list")
{
    WeeklyQuests().Reset();

    CHECK(Add(MakePacket(0, 2, "a", 3)));
    CHECK_FALSE(WeeklyQuests().IsAvailable());
    CHECK(Add(MakePacket(1, 2, "b")));
    CHECK(WeeklyQuests().IsAvailable());

    const auto& quests = WeeklyQuests().GetQuests();
    REQUIRE(quests.size() == 2);
    CHECK(quests[0].Id == L"a");
    CHECK(quests[0].Name == L"Quest a");
    CHECK(quests[0].CurrentCount == 3);
    CHECK(quests[0].RequiredCount == 10);
    CHECK(WeeklyQuests().GetTimeUntilReset().count() > 3500);
}

TEST_CASE("weekly quest catalog rejects an out-of-order list")
{
    WeeklyQuests().Reset();

    CHECK_FALSE(Add(MakePacket(1, 2, "b")));
    CHECK(WeeklyQuests().GetQuests().empty());
    CHECK_FALSE(WeeklyQuests().IsAvailable());
}

TEST_CASE("weekly quest catalog accepts an empty list")
{
    WeeklyQuests().Reset();
    CHECK(Add(MakePacket(0, 1, "a")));

    CHECK(Add(MakePacket(0, 0, "")));
    CHECK(WeeklyQuests().IsAvailable());
    CHECK(WeeklyQuests().GetQuests().empty());
}

TEST_CASE("weekly quest catalog updates a known quest")
{
    WeeklyQuests().Reset();
    CHECK(Add(MakePacket(0, 1, "a", 3)));
    const auto revision = WeeklyQuests().GetRevision();

    auto update = MakePacket(0, 1, "a", 10, true);
    update[IsCompletedOffset] = 1;
    CHECK(Add(update));

    const auto& quest = WeeklyQuests().GetQuests().front();
    CHECK(quest.CurrentCount == 10);
    CHECK(quest.IsCompleted);
    CHECK(quest.IsRewardPending());
    CHECK(WeeklyQuests().GetRevision() != revision);
}

TEST_CASE("weekly quest catalog ignores an update of an unknown quest")
{
    WeeklyQuests().Reset();
    CHECK(Add(MakePacket(0, 1, "a", 3)));

    CHECK_FALSE(Add(MakePacket(0, 1, "b", 5, true)));
    CHECK(WeeklyQuests().GetQuests().size() == 1);
    CHECK(WeeklyQuests().GetQuests().front().CurrentCount == 3);
}

TEST_CASE("weekly quest catalog rejects a message which is too short")
{
    WeeklyQuests().Reset();
    const auto packet = MakePacket(0, 1, "a");

    CHECK_FALSE(WeeklyQuests().AddFromPacket(packet.data(), static_cast<int32_t>(PacketSize - 1)));
    CHECK(WeeklyQuests().GetQuests().empty());
}
