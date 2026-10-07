#include "GameLogic/Quests/SeasonPassCatalog.h"

#include "Data/Translation/MultiLanguage.h"

#include <algorithm>

namespace GameLogic::Quests
{
namespace
{
// Offsets of the SeasonPassState message (C2 header with sub code), which is
// followed by one SeasonPassLevel structure per level.
constexpr int32_t IsPremiumOffset = 5;
constexpr int32_t LevelOffset = 6;
constexpr int32_t MaximumLevelOffset = 8;
constexpr int32_t LevelCountOffset = 10;
constexpr int32_t ExperienceInLevelOffset = 12;
constexpr int32_t ExperiencePerLevelOffset = 16;
constexpr int32_t SecondsUntilEndOffset = 20;
constexpr int32_t SeasonNameOffset = 24;
constexpr int32_t SeasonNameLength = 32;
constexpr int32_t LevelsOffset = 56;

// Offsets within one SeasonPassLevel structure.
constexpr int32_t LevelSize = 132;
constexpr int32_t EntryLevelOffset = 0;
constexpr int32_t EntryIsFreeClaimedOffset = 2;
constexpr int32_t EntryIsPremiumClaimedOffset = 3;
constexpr int32_t EntryFreeRewardsOffset = 4;
constexpr int32_t EntryPremiumRewardsOffset = 68;
constexpr int32_t RewardsLength = 64;

WORD ReadWord(const BYTE* data, int32_t offset)
{
    return static_cast<WORD>(data[offset] | (data[offset + 1] << 8));
}

uint32_t ReadUInt32(const BYTE* data, int32_t offset)
{
    return static_cast<uint32_t>(data[offset]) | (static_cast<uint32_t>(data[offset + 1]) << 8) |
           (static_cast<uint32_t>(data[offset + 2]) << 16) | (static_cast<uint32_t>(data[offset + 3]) << 24);
}

// The strings are UTF-8 and padded with zeros.
std::wstring ReadString(const BYTE* data, int32_t offset, int32_t length)
{
    std::vector<wchar_t> buffer(static_cast<size_t>(length) + 1, L'\0');
    CMultiLanguage::ConvertFromUtf8(buffer.data(), reinterpret_cast<const char*>(data + offset), length);
    buffer[length] = L'\0';
    return std::wstring(buffer.data());
}

SeasonPassLevel ReadLevel(const BYTE* entry)
{
    SeasonPassLevel level;
    level.Level = ReadWord(entry, EntryLevelOffset);
    level.IsFreeClaimed = entry[EntryIsFreeClaimedOffset] != 0;
    level.IsPremiumClaimed = entry[EntryIsPremiumClaimedOffset] != 0;
    level.FreeRewards = ReadString(entry, EntryFreeRewardsOffset, RewardsLength);
    level.PremiumRewards = ReadString(entry, EntryPremiumRewardsOffset, RewardsLength);
    return level;
}
} // namespace

SeasonPassCatalog& SeasonPassCatalog::Instance()
{
    static SeasonPassCatalog instance;
    return instance;
}

void SeasonPassCatalog::Reset()
{
    *this = SeasonPassCatalog();
}

bool SeasonPassCatalog::AddFromPacket(const BYTE* data, int32_t size)
{
    if (data == nullptr || size < LevelsOffset)
    {
        return false;
    }

    const int32_t count = ReadWord(data, LevelCountOffset);
    if (size < LevelsOffset + count * LevelSize)
    {
        return false;
    }

    m_isAvailable = true;
    m_isPremium = data[IsPremiumOffset] != 0;
    m_level = ReadWord(data, LevelOffset);
    m_maximumLevel = ReadWord(data, MaximumLevelOffset);
    m_experienceInLevel = ReadUInt32(data, ExperienceInLevelOffset);
    m_experiencePerLevel = ReadUInt32(data, ExperiencePerLevelOffset);
    m_endsAt = Clock::now() + std::chrono::seconds(ReadUInt32(data, SecondsUntilEndOffset));
    m_seasonName = ReadString(data, SeasonNameOffset, SeasonNameLength);

    m_levels.clear();
    m_levels.reserve(count);
    for (int32_t i = 0; i < count; ++i)
    {
        m_levels.push_back(ReadLevel(data + LevelsOffset + i * LevelSize));
    }

    ++m_revision;
    return true;
}

std::chrono::seconds SeasonPassCatalog::GetTimeUntilEnd() const
{
    const auto remaining = std::chrono::duration_cast<std::chrono::seconds>(m_endsAt - Clock::now());
    return (std::max)(remaining, std::chrono::seconds::zero());
}

bool SeasonPassCatalog::IsClaimable(const SeasonPassLevel& level) const
{
    if (level.Level > m_level)
    {
        return false;
    }

    const bool freeOpen = !level.IsFreeClaimed && !level.FreeRewards.empty();
    const bool premiumOpen = m_isPremium && !level.IsPremiumClaimed && !level.PremiumRewards.empty();
    return freeOpen || premiumOpen;
}

bool SeasonPassCatalog::HasClaimableRewards() const
{
    return std::any_of(m_levels.begin(), m_levels.end(),
                       [this](const SeasonPassLevel& level) { return IsClaimable(level); });
}
} // namespace GameLogic::Quests
