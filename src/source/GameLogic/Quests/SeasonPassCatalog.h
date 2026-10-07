#pragma once

#include "Core/Platform/WinCompat.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

// The season pass of the account, as the server sends it with the
// SeasonPassState message: the running season, the reached level, the
// experience and the rewards of each level. Servers which don't know the pass
// never send it, so nothing is known and the window shows no pass.
namespace GameLogic::Quests
{
struct SeasonPassLevel
{
    int Level = 0;
    bool IsFreeClaimed = false;
    bool IsPremiumClaimed = false;
    std::wstring FreeRewards;
    std::wstring PremiumRewards;
};

class SeasonPassCatalog
{
public:
    using Clock = std::chrono::steady_clock;

    static SeasonPassCatalog& Instance();

    // Forgets everything, e.g. when we connect to another server.
    void Reset();

    // Takes the SeasonPassState message. Returns false when the data isn't
    // plausible, so that the caller can ignore it.
    bool AddFromPacket(const BYTE* data, int32_t size);

    // The server told us about the pass, with or without a running season.
    bool IsAvailable() const { return m_isAvailable; }
    // A season is running.
    bool HasSeason() const { return !m_seasonName.empty(); }

    const std::wstring& GetSeasonName() const { return m_seasonName; }
    bool IsPremium() const { return m_isPremium; }
    int GetLevel() const { return m_level; }
    int GetMaximumLevel() const { return m_maximumLevel; }
    uint32_t GetExperienceInLevel() const { return m_experienceInLevel; }
    uint32_t GetExperiencePerLevel() const { return m_experiencePerLevel; }
    const std::vector<SeasonPassLevel>& GetLevels() const { return m_levels; }
    std::chrono::seconds GetTimeUntilEnd() const;

    // The rewards of a reached level which haven't been received yet.
    bool IsClaimable(const SeasonPassLevel& level) const;
    bool HasClaimableRewards() const;

    // Counts up with every message, so that a window knows when to refresh.
    uint32_t GetRevision() const { return m_revision; }

private:
    bool m_isAvailable = false;
    std::wstring m_seasonName;
    bool m_isPremium = false;
    int m_level = 0;
    int m_maximumLevel = 0;
    uint32_t m_experienceInLevel = 0;
    uint32_t m_experiencePerLevel = 0;
    Clock::time_point m_endsAt{};
    std::vector<SeasonPassLevel> m_levels;
    uint32_t m_revision = 0;
};

inline SeasonPassCatalog& SeasonPass()
{
    return SeasonPassCatalog::Instance();
}
} // namespace GameLogic::Quests
