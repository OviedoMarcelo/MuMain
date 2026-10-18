#include "GameLogic/Cosmetics/CosmeticPieceCatalog.h"

#include "Data/Translation/MultiLanguage.h"

#include <algorithm>

namespace GameLogic::Cosmetics
{
namespace
{
// Offsets of the CosmeticPieces message (C2 header with sub code), which is
// followed by one CosmeticPiece structure per piece.
constexpr int32_t PieceCountOffset = 6;
constexpr int32_t PiecesOffset = 8;

// Offsets within one CosmeticPiece structure.
constexpr int32_t PieceSize = 42;
constexpr int32_t EntryNumberOffset = 0;
constexpr int32_t EntryItemNumberOffset = 2;
constexpr int32_t EntryItemGroupOffset = 4;
constexpr int32_t EntryLevelOffset = 5;
constexpr int32_t EntryIsExcellentOffset = 6;
constexpr int32_t EntryIsUnlockedOffset = 7;
constexpr int32_t EntryCanWearOffset = 8;
constexpr int32_t EntryIsWornOffset = 9;
constexpr int32_t EntryNameOffset = 10;
constexpr int32_t NameLength = 32;

WORD ReadWord(const BYTE* data, int32_t offset)
{
    return static_cast<WORD>(data[offset] | (data[offset + 1] << 8));
}

// The strings are UTF-8 and padded with zeros.
std::wstring ReadString(const BYTE* data, int32_t offset, int32_t length)
{
    std::vector<wchar_t> buffer(static_cast<size_t>(length) + 1, L'\0');
    CMultiLanguage::ConvertFromUtf8(buffer.data(), reinterpret_cast<const char*>(data + offset), length);
    buffer[length] = L'\0';
    return std::wstring(buffer.data());
}

CosmeticPiece ReadPiece(const BYTE* entry)
{
    CosmeticPiece piece;
    piece.Number = ReadWord(entry, EntryNumberOffset);
    piece.ItemNumber = ReadWord(entry, EntryItemNumberOffset);
    piece.ItemGroup = entry[EntryItemGroupOffset];
    piece.Level = entry[EntryLevelOffset];
    piece.IsExcellent = entry[EntryIsExcellentOffset] != 0;
    piece.IsUnlocked = entry[EntryIsUnlockedOffset] != 0;
    piece.CanWear = entry[EntryCanWearOffset] != 0;
    piece.IsWorn = entry[EntryIsWornOffset] != 0;
    piece.Name = ReadString(entry, EntryNameOffset, NameLength);
    return piece;
}
} // namespace

CosmeticPieceCatalog& CosmeticPieceCatalog::Instance()
{
    static CosmeticPieceCatalog instance;
    return instance;
}

void CosmeticPieceCatalog::Reset()
{
    // The revision keeps counting, so that a window which showed the old pieces refreshes.
    const auto revision = m_revision;
    *this = CosmeticPieceCatalog();
    m_revision = revision + 1;
}

bool CosmeticPieceCatalog::AddFromPacket(const BYTE* data, int32_t size)
{
    if (data == nullptr || size < PiecesOffset)
    {
        return false;
    }

    const int32_t count = ReadWord(data, PieceCountOffset);
    if (size < PiecesOffset + count * PieceSize)
    {
        return false;
    }

    m_isAvailable = true;
    m_pieces.clear();
    m_pieces.reserve(count);
    for (int32_t i = 0; i < count; ++i)
    {
        m_pieces.push_back(ReadPiece(data + PiecesOffset + i * PieceSize));
    }

    ++m_revision;
    return true;
}

const CosmeticPiece* CosmeticPieceCatalog::GetWornPiece(BYTE itemGroup) const
{
    const auto it = std::find_if(m_pieces.begin(), m_pieces.end(),
                                 [itemGroup](const CosmeticPiece& piece) { return piece.IsWorn && piece.ItemGroup == itemGroup; });
    return it == m_pieces.end() ? nullptr : &*it;
}
} // namespace GameLogic::Cosmetics
