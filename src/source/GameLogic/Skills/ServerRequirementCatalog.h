#pragma once

#include "Core/Platform/WinCompat.h"

#include <cstdint>
#include <unordered_map>

// The requirements of the skills, and of the items which teach them, as the
// server checks them.
//
// The server sends them once after the login, with the SkillRequirements and
// LearnableItemRequirements messages. From then on they replace the values of
// our own data files (Skill_*.bmd, Item_*.bmd), so that we show and check
// exactly what the server checks. Until they arrive, and with servers which
// don't send them, the values of the data files apply. All values are the ones
// which are compared with the total stats of the character, without formulas.
namespace GameLogic::Skills
{
struct SkillRequirement
{
    WORD Level = 0;
    WORD Energy = 0;
    WORD Leadership = 0;
    WORD Strength = 0;
    WORD Agility = 0;
    WORD Mana = 0;
    WORD AbilityGauge = 0;
};

// What it takes to learn the skill of an item (orb, scroll, parchment, crystal).
struct ItemRequirement
{
    WORD Level = 0;
    WORD Energy = 0;
    WORD Leadership = 0;
    WORD Strength = 0;
    WORD Agility = 0;
};

class ServerRequirementCatalog
{
public:
    static ServerRequirementCatalog& Instance();

    // Forgets everything, e.g. when we connect to another server.
    void Reset();

    // Takes the SkillRequirements message. Returns false when the data isn't
    // plausible, so that the caller can ignore it.
    bool AddSkillsFromPacket(const BYTE* data, int32_t size);

    // Takes the LearnableItemRequirements message. Returns false when the data
    // isn't plausible, so that the caller can ignore it.
    bool AddItemsFromPacket(const BYTE* data, int32_t size);

    // The requirements of the skill, or nullptr if the server didn't send them.
    const SkillRequirement* FindSkill(int skillType) const;

    // The requirements to learn the skill of the item (group * MAX_ITEM_INDEX + number)
    // with the item level, or nullptr if the server didn't send them.
    const ItemRequirement* FindItem(int itemType, int itemLevel) const;

private:
    std::unordered_map<int, SkillRequirement> m_skills;
    std::unordered_map<int, ItemRequirement> m_items;
};

inline ServerRequirementCatalog& ServerRequirements()
{
    return ServerRequirementCatalog::Instance();
}
} // namespace GameLogic::Skills
