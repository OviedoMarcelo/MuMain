#include "GameLogic/Cosmetics/CosmeticSetCatalog.h"

#include "Data/Translation/MultiLanguage.h"

#include <algorithm>

namespace GameLogic::Cosmetics
{
namespace
{
// Offsets of the CosmeticSets message (C2 header with sub code), which is
// followed by one CosmeticSet structure per set.
constexpr int32_t SetCountOffset = 5;
constexpr int32_t WornSetNumberOffset = 6;
constexpr int32_t SetsOffset = 8;

// Offsets within one CosmeticSet structure.
constexpr int32_t SetSize = 40;
constexpr int32_t EntryNumberOffset = 0;
constexpr int32_t EntryItemNumberOffset = 2;
constexpr int32_t EntryLevelOffset = 4;
constexpr int32_t EntryIsExcellentOffset = 5;
constexpr int32_t EntryIsUnlockedOffset = 6;
constexpr int32_t EntryCanWearOffset = 7;
constexpr int32_t EntryNameOffset = 8;
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

CosmeticSet ReadSet(const BYTE* entry)
{
    CosmeticSet set;
    set.Number = ReadWord(entry, EntryNumberOffset);
    set.ItemNumber = ReadWord(entry, EntryItemNumberOffset);
    set.Level = entry[EntryLevelOffset];
    set.IsExcellent = entry[EntryIsExcellentOffset] != 0;
    set.IsUnlocked = entry[EntryIsUnlockedOffset] != 0;
    set.CanWear = entry[EntryCanWearOffset] != 0;
    set.Name = ReadString(entry, EntryNameOffset, NameLength);
    return set;
}
} // namespace

CosmeticSetCatalog& CosmeticSetCatalog::Instance()
{
    static CosmeticSetCatalog instance;
    return instance;
}

void CosmeticSetCatalog::Reset()
{
    // The revision keeps counting, so that a window which showed the old sets refreshes.
    const auto revision = m_revision;
    *this = CosmeticSetCatalog();
    m_revision = revision + 1;
}

bool CosmeticSetCatalog::AddFromPacket(const BYTE* data, int32_t size)
{
    if (data == nullptr || size < SetsOffset)
    {
        return false;
    }

    const int32_t count = data[SetCountOffset];
    if (size < SetsOffset + count * SetSize)
    {
        return false;
    }

    m_isAvailable = true;
    m_wornSetNumber = ReadWord(data, WornSetNumberOffset);
    m_sets.clear();
    m_sets.reserve(count);
    for (int32_t i = 0; i < count; ++i)
    {
        m_sets.push_back(ReadSet(data + SetsOffset + i * SetSize));
    }

    ++m_revision;
    return true;
}

const CosmeticSet* CosmeticSetCatalog::GetWornSet() const
{
    if (m_wornSetNumber == NoSet)
    {
        return nullptr;
    }

    const auto it = std::find_if(m_sets.begin(), m_sets.end(), [this](const CosmeticSet& set) { return set.Number == m_wornSetNumber; });
    return it == m_sets.end() ? nullptr : &*it;
}
} // namespace GameLogic::Cosmetics
