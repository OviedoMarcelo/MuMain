#pragma once

#include "UI/NewUI/Quests/RisingParticles.h"

#include <cstdint>

// The particles which are drawn over the portrait of an NPC in the dialog of a
// quest. Like the portraits, they belong to the NPC: an NPC without an entry in
// the table of NpcPortraitEffects.cpp has none.
namespace UI::Quests::NpcPortraitEffects
{
// Little golden lights which rise slowly, e.g. over Elf Lala.
extern const RisingParticleStyle GoldenMotes;
// Thin sparks of fire which fly up in a fan and flicker, from orange to red, e.g. over the guards.
extern const RisingParticleStyle FireSparks;
// Sparkles of blue magic which float up, waving, and twinkle, e.g. over Marce.
extern const RisingParticleStyle ArcaneSparkles;

// The particles of the NPC; nullptr when it has none.
const RisingParticleStyle* ForNpc(uint16_t npcNumber);
} // namespace UI::Quests::NpcPortraitEffects
