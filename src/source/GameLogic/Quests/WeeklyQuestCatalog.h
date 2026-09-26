#pragma once

#include "Core/Platform/WinCompat.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

// The weekly quests which the server offers to this player, with their progress.
//
// The server sends one WeeklyQuestEntry message per quest when we ask for the
// chat commands after entering the map, and a single message as update when
// the progress of a quest changes. The texts come from the server, because the
// quests are configured there at runtime.
namespace GameLogic::Quests
{
struct WeeklyQuest
{
    std::wstring Id;
    std::wstring Name;
    std::wstring Description;
    std::wstring Rewards;
    uint32_t CurrentCount = 0;
    uint32_t RequiredCount = 0;
    bool IsCompleted = false;
    // A completed quest which is not rewarded yet waits for free inventory space.
    bool IsRewarded = false;

    bool IsRewardPending() const
    {
        return IsCompleted && !IsRewarded;
    }
};

class WeeklyQuestCatalog
{
public:
    using Clock = std::chrono::steady_clock;

    static WeeklyQuestCatalog& Instance();

    const std::vector<WeeklyQuest>& GetQuests() const
    {
        return m_quests;
    }

    // True when the server told us about the weekly quests, even if there are
    // none this week. Until then the feature stays hidden - the server may be
    // one which doesn't know them.
    bool IsAvailable() const
    {
        return m_isComplete;
    }

    // Changes whenever the quests or their progress changed, so that a window
    // knows when to refresh what it derived from them.
    uint32_t GetRevision() const
    {
        return m_revision;
    }

    // The time which is left until the weekly progress is reset.
    std::chrono::seconds GetTimeUntilReset() const;

    // Forgets everything, e.g. when the character changed.
    void Reset();

    // Takes one WeeklyQuestEntry message. Returns false when the data isn't
    // plausible, so that the caller can ignore it.
    bool AddFromPacket(const BYTE* data, int32_t size);

private:
    bool AddToList(WeeklyQuest quest, BYTE index, BYTE count);
    bool Update(WeeklyQuest quest);

    std::vector<WeeklyQuest> m_quests;
    BYTE m_expectedCount = 0;
    bool m_isComplete = false;
    uint32_t m_revision = 0;
    Clock::time_point m_resetTime = {};
};

inline WeeklyQuestCatalog& WeeklyQuests()
{
    return WeeklyQuestCatalog::Instance();
}
} // namespace GameLogic::Quests
