#include "Core/Platform/WinCompat.h"
#include "Core/Globals/_define.h"
#include "GameLogic/Skills/ServerRequirementCatalog.h"

#include "doctest.h"

#include <vector>

namespace
{
using GameLogic::Skills::ServerRequirements;

constexpr size_t CountOffset = 6;
constexpr size_t EntriesOffset = 8;
constexpr size_t EntrySize = 16;

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

// A SkillRequirement entry: number, level, energy, leadership, strength, agility, mana, AG.
void WriteSkill(std::vector<BYTE>& packet, size_t index, WORD number, WORD energy, WORD mana)
{
    const auto offset = EntriesOffset + index * EntrySize;
    WriteWord(packet, offset + 0, number);
    WriteWord(packet, offset + 2, 10);
    WriteWord(packet, offset + 4, energy);
    WriteWord(packet, offset + 12, mana);
    WriteWord(packet, offset + 14, 5);
}

// A LearnableItemRequirement entry: group, item level, number, level, energy, leadership, strength, agility, skill.
void WriteItem(std::vector<BYTE>& packet, size_t index, BYTE group, BYTE itemLevel, WORD number, WORD energy)
{
    const auto offset = EntriesOffset + index * EntrySize;
    packet[offset + 0] = group;
    packet[offset + 1] = itemLevel;
    WriteWord(packet, offset + 2, number);
    WriteWord(packet, offset + 6, energy);
}
} // namespace

TEST_CASE("server requirement catalog reads the skills")
{
    ServerRequirements().Reset();
    auto packet = MakePacket(2);
    WriteSkill(packet, 0, 30, 90, 40);
    WriteSkill(packet, 1, 31, 170, 70);

    REQUIRE(ServerRequirements().AddSkillsFromPacket(packet.data(), static_cast<int32_t>(packet.size())));

    const auto* goblin = ServerRequirements().FindSkill(30);
    REQUIRE(goblin != nullptr);
    CHECK(goblin->Energy == 90);
    CHECK(goblin->Mana == 40);
    CHECK(goblin->Level == 10);
    CHECK(goblin->AbilityGauge == 5);
    CHECK(ServerRequirements().FindSkill(31)->Energy == 170);
    CHECK(ServerRequirements().FindSkill(32) == nullptr);
}

TEST_CASE("server requirement catalog rejects a message which is shorter than its count")
{
    ServerRequirements().Reset();
    auto packet = MakePacket(2);
    WriteWord(packet, CountOffset, 3);

    CHECK_FALSE(ServerRequirements().AddSkillsFromPacket(packet.data(), static_cast<int32_t>(packet.size())));
    CHECK_FALSE(ServerRequirements().AddSkillsFromPacket(nullptr, 0));
    CHECK(ServerRequirements().FindSkill(0) == nullptr);
}

TEST_CASE("server requirement catalog ignores skills beyond the ones the client knows")
{
    ServerRequirements().Reset();
    auto packet = MakePacket(1);
    WriteSkill(packet, 0, MAX_SKILLS, 1, 1);

    CHECK(ServerRequirements().AddSkillsFromPacket(packet.data(), static_cast<int32_t>(packet.size())));
    CHECK(ServerRequirements().FindSkill(MAX_SKILLS) == nullptr);
}

TEST_CASE("server requirement catalog finds items by level, or for every level")
{
    ServerRequirements().Reset();
    auto packet = MakePacket(3);
    WriteItem(packet, 0, 12, 0, 11, 30);   // Orb of Summoning +0
    WriteItem(packet, 1, 12, 1, 11, 60);   // Orb of Summoning +1
    WriteItem(packet, 2, 15, 0xFF, 3, 40); // Scroll of Fire Ball, any level

    REQUIRE(ServerRequirements().AddItemsFromPacket(packet.data(), static_cast<int32_t>(packet.size())));

    const int orb = 12 * MAX_ITEM_INDEX + 11;
    const int scroll = 15 * MAX_ITEM_INDEX + 3;
    CHECK(ServerRequirements().FindItem(orb, 0)->Energy == 30);
    CHECK(ServerRequirements().FindItem(orb, 1)->Energy == 60);
    CHECK(ServerRequirements().FindItem(orb, 2) == nullptr);
    CHECK(ServerRequirements().FindItem(scroll, 0)->Energy == 40);
    CHECK(ServerRequirements().FindItem(scroll, 7)->Energy == 40);
}

TEST_CASE("server requirement catalog forgets everything on reset")
{
    auto packet = MakePacket(1);
    WriteSkill(packet, 0, 30, 90, 40);
    REQUIRE(ServerRequirements().AddSkillsFromPacket(packet.data(), static_cast<int32_t>(packet.size())));

    ServerRequirements().Reset();

    CHECK(ServerRequirements().FindSkill(30) == nullptr);
}
