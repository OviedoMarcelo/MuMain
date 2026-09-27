#pragma once

#include "Core/Platform/WinCompat.h"

#include <cstdint>
#include <unordered_map>

// The level of the monsters, as the server configures them.
//
// Our data files only have the name of each monster, so the server sends their
// levels once after the login, with the MonsterLevels message. Until they arrive,
// and with servers which don't send them, no level is known and none is shown.
namespace GameLogic::Monsters
{
// How hard a monster is for the hero, by the difference of their levels.
enum class LevelDifficulty
{
    Trivial, // 10 or more levels below the hero
    Even,    // from 9 levels below to 4 levels above
    Hard,    // from 5 to 9 levels above
    Deadly,  // 10 or more levels above
};

LevelDifficulty GetLevelDifficulty(int monsterLevel, int heroLevel);

class ServerMonsterLevelCatalog
{
public:
    static ServerMonsterLevelCatalog& Instance();

    // Forgets everything, e.g. when we connect to another server.
    void Reset();

    // Takes the MonsterLevels message. Returns false when the data isn't
    // plausible, so that the caller can ignore it.
    bool AddFromPacket(const BYTE* data, int32_t size);

    // The level of the monster type, or -1 if the server didn't send it.
    int Find(int monsterType) const;

private:
    std::unordered_map<int, int> m_levels;
};

inline ServerMonsterLevelCatalog& ServerMonsterLevels()
{
    return ServerMonsterLevelCatalog::Instance();
}
} // namespace GameLogic::Monsters
