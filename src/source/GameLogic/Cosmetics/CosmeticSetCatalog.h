#pragma once

#include "Core/Platform/WinCompat.h"

#include <cstdint>
#include <string>
#include <vector>

// The cosmetic sets of the own character, as the server sends them with the
// CosmeticSets message: the configured sets, which ones the character unlocked,
// and the one it wears over its armor. Servers which don't know cosmetic sets
// never send it, so nothing is known and the window shows no sets.
namespace GameLogic::Cosmetics
{
struct CosmeticSet
{
    WORD Number = 0;
    // The number of the pieces in the item groups of helms, armors, pants, gloves and boots.
    WORD ItemNumber = 0;
    BYTE Level = 0;
    bool IsExcellent = false;
    bool IsUnlocked = false;
    // The class of the character can wear the set.
    bool CanWear = false;
    std::wstring Name;
};

class CosmeticSetCatalog
{
public:
    // The number which the server sends when the character wears no set.
    static constexpr WORD NoSet = 0;

    static CosmeticSetCatalog& Instance();

    // Forgets everything, e.g. when we connect to another server.
    void Reset();

    // Takes the CosmeticSets message. Returns false when the data isn't
    // plausible, so that the caller can ignore it.
    bool AddFromPacket(const BYTE* data, int32_t size);

    // The server told us about the cosmetic sets.
    bool IsAvailable() const { return m_isAvailable; }

    const std::vector<CosmeticSet>& GetSets() const { return m_sets; }

    // The set which the character wears; nullptr, if it wears none.
    const CosmeticSet* GetWornSet() const;

    // Counts up with every message, so that a window knows when to refresh.
    uint32_t GetRevision() const { return m_revision; }

private:
    bool m_isAvailable = false;
    WORD m_wornSetNumber = NoSet;
    std::vector<CosmeticSet> m_sets;
    uint32_t m_revision = 0;
};

inline CosmeticSetCatalog& CosmeticSets()
{
    return CosmeticSetCatalog::Instance();
}
} // namespace GameLogic::Cosmetics
