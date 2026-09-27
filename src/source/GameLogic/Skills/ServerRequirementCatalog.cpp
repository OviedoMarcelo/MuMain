#include "GameLogic/Skills/ServerRequirementCatalog.h"

#include "Core/Globals/_define.h"

namespace GameLogic::Skills
{
namespace
{
// Both messages have a C2 header with sub code, a count and a list of entries.
constexpr int32_t CountOffset = 6;
constexpr int32_t EntriesOffset = 8;
constexpr int32_t EntrySize = 16;

// Offsets within one SkillRequirement entry.
constexpr int32_t SkillNumberOffset = 0;
constexpr int32_t SkillLevelOffset = 2;
constexpr int32_t SkillEnergyOffset = 4;
constexpr int32_t SkillLeadershipOffset = 6;
constexpr int32_t SkillStrengthOffset = 8;
constexpr int32_t SkillAgilityOffset = 10;
constexpr int32_t SkillManaOffset = 12;
constexpr int32_t SkillAbilityGaugeOffset = 14;

// Offsets within one LearnableItemRequirement entry.
constexpr int32_t ItemGroupOffset = 0;
constexpr int32_t ItemLevelOffset = 1;
constexpr int32_t ItemNumberOffset = 2;
constexpr int32_t ItemRequiredLevelOffset = 4;
constexpr int32_t ItemEnergyOffset = 6;
constexpr int32_t ItemLeadershipOffset = 8;
constexpr int32_t ItemStrengthOffset = 10;
constexpr int32_t ItemAgilityOffset = 12;

// The item level of requirements which apply to every level of the item.
constexpr int AnyItemLevel = 0xFF;

WORD ReadWord(const BYTE* data, int32_t offset)
{
    return static_cast<WORD>(data[offset] | (data[offset + 1] << 8));
}

// The number of entries, or -1 if the message is too short for them.
int32_t ReadEntryCount(const BYTE* data, int32_t size)
{
    if (data == nullptr || size < EntriesOffset)
    {
        return -1;
    }

    const int32_t count = ReadWord(data, CountOffset);
    return size < EntriesOffset + count * EntrySize ? -1 : count;
}

int GetItemKey(int itemType, int itemLevel)
{
    return itemType * 256 + itemLevel;
}
} // namespace

ServerRequirementCatalog& ServerRequirementCatalog::Instance()
{
    static ServerRequirementCatalog instance;
    return instance;
}

void ServerRequirementCatalog::Reset()
{
    this->m_skills.clear();
    this->m_items.clear();
}

bool ServerRequirementCatalog::AddSkillsFromPacket(const BYTE* data, int32_t size)
{
    const auto count = ReadEntryCount(data, size);
    if (count < 0)
    {
        return false;
    }

    this->m_skills.clear();
    for (int32_t i = 0; i < count; ++i)
    {
        const auto* entry = data + EntriesOffset + i * EntrySize;
        const int skillType = ReadWord(entry, SkillNumberOffset);
        if (skillType >= MAX_SKILLS)
        {
            continue;
        }

        SkillRequirement requirement;
        requirement.Level = ReadWord(entry, SkillLevelOffset);
        requirement.Energy = ReadWord(entry, SkillEnergyOffset);
        requirement.Leadership = ReadWord(entry, SkillLeadershipOffset);
        requirement.Strength = ReadWord(entry, SkillStrengthOffset);
        requirement.Agility = ReadWord(entry, SkillAgilityOffset);
        requirement.Mana = ReadWord(entry, SkillManaOffset);
        requirement.AbilityGauge = ReadWord(entry, SkillAbilityGaugeOffset);
        this->m_skills[skillType] = requirement;
    }

    return true;
}

bool ServerRequirementCatalog::AddItemsFromPacket(const BYTE* data, int32_t size)
{
    const auto count = ReadEntryCount(data, size);
    if (count < 0)
    {
        return false;
    }

    this->m_items.clear();
    for (int32_t i = 0; i < count; ++i)
    {
        const auto* entry = data + EntriesOffset + i * EntrySize;
        const int group = entry[ItemGroupOffset];
        const int number = ReadWord(entry, ItemNumberOffset);
        if (group >= MAX_ITEM_TYPE || number >= MAX_ITEM_INDEX)
        {
            continue;
        }

        ItemRequirement requirement;
        requirement.Level = ReadWord(entry, ItemRequiredLevelOffset);
        requirement.Energy = ReadWord(entry, ItemEnergyOffset);
        requirement.Leadership = ReadWord(entry, ItemLeadershipOffset);
        requirement.Strength = ReadWord(entry, ItemStrengthOffset);
        requirement.Agility = ReadWord(entry, ItemAgilityOffset);
        this->m_items[GetItemKey(group * MAX_ITEM_INDEX + number, entry[ItemLevelOffset])] = requirement;
    }

    return true;
}

const SkillRequirement* ServerRequirementCatalog::FindSkill(int skillType) const
{
    const auto found = this->m_skills.find(skillType);
    return found == this->m_skills.end() ? nullptr : &found->second;
}

const ItemRequirement* ServerRequirementCatalog::FindItem(int itemType, int itemLevel) const
{
    auto found = this->m_items.find(GetItemKey(itemType, itemLevel));
    if (found == this->m_items.end())
    {
        found = this->m_items.find(GetItemKey(itemType, AnyItemLevel));
    }

    return found == this->m_items.end() ? nullptr : &found->second;
}
} // namespace GameLogic::Skills
