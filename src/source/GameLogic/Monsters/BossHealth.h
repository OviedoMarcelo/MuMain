#pragma once

#include "Core/Platform/WinCompat.h"

#include <cstdint>
#include <optional>
#include <string>

// The health of the boss of an encounter, which the players in its arena see
// in a bar.
//
// The server sends a BossHealth message when the player enters the arena and
// when the health or the phase of the boss changes. It sends it with the state
// "hidden" when the player leaves the arena or the encounter ends. Servers
// which don't know the message never send it, so no bar is shown.
namespace GameLogic::Monsters
{
struct BossHealth
{
    int Key = 0;
    bool IsVisible = false;
    int HealthPercent = 0;
    std::wstring BossName;

    // Empty without phases.
    std::wstring PhaseName;
};

// Reads the BossHealth message, or nothing when the data isn't plausible, so
// that the caller can ignore it.
std::optional<BossHealth> ReadBossHealth(const BYTE* data, int32_t size);
} // namespace GameLogic::Monsters
