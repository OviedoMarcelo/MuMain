#pragma once

#include "Core/Platform/WinCompat.h"

#include <cstdint>
#include <optional>
#include <string>

// How a single monster looks different than the others of its kind, e.g. the
// boss of an encounter: bigger, glowing and with a name of its own.
//
// The server sends a MonsterAppearance message right after the monster came
// into view. The monster looks so until it leaves the view, because a new
// AddNpcsToScope message creates it anew. Servers which don't know the
// message never send it, so all monsters look like always.
namespace GameLogic::Monsters
{
// The glow, like the glow of an item of that level. The values are the ones
// of the message.
enum class MonsterGlow : BYTE
{
    None = 0,
    Level7 = 7,
    Level9 = 9,
    Level11 = 11,
    Level13 = 13,
    Level15 = 15,
    Golden = 100,
};

struct MonsterAppearance
{
    int Key = 0;
    int ScalePercent = 100;
    MonsterGlow Glow = MonsterGlow::None;

    // Empty for the name of the monster.
    std::wstring Name;
};

// Reads the MonsterAppearance message, or nothing when the data isn't
// plausible, so that the caller can ignore it. An unknown glow is no glow.
std::optional<MonsterAppearance> ReadMonsterAppearance(const BYTE* data, int32_t size);
} // namespace GameLogic::Monsters
