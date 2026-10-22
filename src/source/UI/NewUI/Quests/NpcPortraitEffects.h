#pragma once

#include <cstdint>

// The effect which is drawn over the portrait of an NPC in the dialog of a quest.
// Like the portraits, they belong to the NPC: an NPC without an entry in the
// table of NpcPortraitEffects.cpp has no effect.
namespace UI::Quests::NpcPortraitEffects
{
enum class Effect
{
    None,
    // Little golden lights which rise slowly over the portrait.
    GoldenMotes,
};

Effect ForNpc(uint16_t npcNumber);
} // namespace UI::Quests::NpcPortraitEffects
