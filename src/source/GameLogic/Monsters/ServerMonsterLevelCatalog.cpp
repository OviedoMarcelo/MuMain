#include "GameLogic/Monsters/ServerMonsterLevelCatalog.h"

namespace GameLogic::Monsters
{
namespace
{
// The message has a C2 header with sub code, a count and a list of entries.
constexpr int32_t CountOffset = 6;
constexpr int32_t EntriesOffset = 8;
constexpr int32_t EntrySize = 4;

// Offsets within one MonsterLevel entry.
constexpr int32_t MonsterNumberOffset = 0;
constexpr int32_t LevelOffset = 2;

WORD ReadWord(const BYTE* data, int32_t offset)
{
    return static_cast<WORD>(data[offset] | (data[offset + 1] << 8));
}
} // namespace

LevelDifficulty GetLevelDifficulty(int monsterLevel, int heroLevel)
{
    const int difference = monsterLevel - heroLevel;
    if (difference <= -10)
    {
        return LevelDifficulty::Trivial;
    }

    if (difference >= 10)
    {
        return LevelDifficulty::Deadly;
    }

    return difference >= 5 ? LevelDifficulty::Hard : LevelDifficulty::Even;
}

ServerMonsterLevelCatalog& ServerMonsterLevelCatalog::Instance()
{
    static ServerMonsterLevelCatalog instance;
    return instance;
}

void ServerMonsterLevelCatalog::Reset()
{
    this->m_levels.clear();
}

bool ServerMonsterLevelCatalog::AddFromPacket(const BYTE* data, int32_t size)
{
    if (data == nullptr || size < EntriesOffset)
    {
        return false;
    }

    const int32_t count = ReadWord(data, CountOffset);
    if (size < EntriesOffset + count * EntrySize)
    {
        return false;
    }

    this->m_levels.clear();
    for (int32_t i = 0; i < count; ++i)
    {
        const auto* entry = data + EntriesOffset + i * EntrySize;
        this->m_levels[ReadWord(entry, MonsterNumberOffset)] = ReadWord(entry, LevelOffset);
    }

    return true;
}

int ServerMonsterLevelCatalog::Find(int monsterType) const
{
    const auto found = this->m_levels.find(monsterType);
    return found == this->m_levels.end() ? -1 : found->second;
}
} // namespace GameLogic::Monsters
