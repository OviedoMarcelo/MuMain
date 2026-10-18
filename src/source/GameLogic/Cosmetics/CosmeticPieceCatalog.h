#pragma once

#include "Core/Platform/WinCompat.h"

#include <cstdint>
#include <string>
#include <vector>

// The cosmetic pieces of the own character, as the server sends them with the
// CosmeticPieces message: the configured pieces, which ones the character
// unlocked, and the ones it wears over its armor, at most one per slot.
// Servers which don't know cosmetic pieces never send it, so nothing is known
// and the window shows no pieces.
namespace GameLogic::Cosmetics
{
struct CosmeticPiece
{
    WORD Number = 0;
    WORD ItemNumber = 0;
    // The item group tells the slot: helms (7), armors (8), pants (9), gloves (10) or boots (11).
    BYTE ItemGroup = 0;
    BYTE Level = 0;
    bool IsExcellent = false;
    bool IsUnlocked = false;
    // The class of the character can wear the piece.
    bool CanWear = false;
    bool IsWorn = false;
    std::wstring Name;
};

class CosmeticPieceCatalog
{
public:
    static CosmeticPieceCatalog& Instance();

    // Forgets everything, e.g. when we connect to another server.
    void Reset();

    // Takes the CosmeticPieces message. Returns false when the data isn't
    // plausible, so that the caller can ignore it.
    bool AddFromPacket(const BYTE* data, int32_t size);

    // The server told us about the cosmetic pieces.
    bool IsAvailable() const { return m_isAvailable; }

    const std::vector<CosmeticPiece>& GetPieces() const { return m_pieces; }

    // The piece which the character wears over the items of a group; nullptr, if it wears none.
    const CosmeticPiece* GetWornPiece(BYTE itemGroup) const;

    // Counts up with every message, so that a window knows when to refresh.
    uint32_t GetRevision() const { return m_revision; }

private:
    bool m_isAvailable = false;
    std::vector<CosmeticPiece> m_pieces;
    uint32_t m_revision = 0;
};

inline CosmeticPieceCatalog& CosmeticPieces()
{
    return CosmeticPieceCatalog::Instance();
}
} // namespace GameLogic::Cosmetics
