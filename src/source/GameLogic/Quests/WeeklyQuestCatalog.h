#pragma once

#include "Core/Platform/WinCompat.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

// The quests which the server offers to this player, with their progress:
// story chapters, daily, weekly, class and zone quests.
//
// The server sends one WeeklyQuestEntry message per quest when we ask for the
// chat commands after entering the map, and a single message as update when
// the progress of a quest changes. Each of them is followed by a QuestDetails
// message with the category and the steps of the quest; older servers don't
// send it, then every quest is a weekly one with a single objective. The texts
// come from the server, because the quests are configured there at runtime.
namespace GameLogic::Quests
{
// The values are the ones which the server sends.
enum class QuestCategory : BYTE
{
    Weekly = 0,
    Daily = 1,
    Main = 2,
    Class = 3,
    Zone = 4,
};

// When the progress of a quest starts over. The values are the ones which the server sends.
enum class QuestPeriod : BYTE
{
    Weekly = 0,
    Daily = 1,
    Once = 2,
};

// An objective (step) of a quest, e.g. "Kill 50 Skeletons".
struct QuestObjective
{
    std::wstring Text;
    uint32_t CurrentCount = 0;
    uint32_t RequiredCount = 0;
    bool IsDone = false;
};

struct WeeklyQuest
{
    using Clock = std::chrono::steady_clock;

    std::wstring Id;
    std::wstring Name;
    std::wstring Description;
    std::wstring Rewards;
    // With several objectives, the counts are the done objectives and their number.
    uint32_t CurrentCount = 0;
    uint32_t RequiredCount = 0;
    bool IsCompleted = false;
    // A completed quest which is not rewarded yet waits for free inventory space.
    bool IsRewarded = false;

    // False as long as no QuestDetails message was received, e.g. from an older server.
    bool HasDetails = false;
    QuestCategory Category = QuestCategory::Weekly;
    QuestPeriod Period = QuestPeriod::Weekly;
    // The objectives have to be done in their order, CurrentStep is the one to do now.
    bool IsSequential = false;
    size_t CurrentStep = 0;
    std::vector<QuestObjective> Objectives;
    Clock::time_point ResetTime = {};

    bool IsRewardPending() const
    {
        return IsCompleted && !IsRewarded;
    }

    // A quest with a single objective shows its progress, one with several shows its steps.
    bool HasSteps() const
    {
        return Objectives.size() > 1;
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

    // The time which is left until the progress of the quest is reset.
    static std::chrono::seconds GetTimeUntilReset(const WeeklyQuest& quest);

    // Forgets everything, e.g. when the character changed.
    void Reset();

    // Takes one WeeklyQuestEntry message. Returns false when the data isn't
    // plausible, so that the caller can ignore it.
    bool AddFromPacket(const BYTE* data, int32_t size);

    // Takes one QuestDetails message, which completes a quest that we already
    // know from its WeeklyQuestEntry message. Returns false when the data isn't
    // plausible or the quest is unknown.
    bool AddDetailsFromPacket(const BYTE* data, int32_t size);

private:
    bool AddToList(WeeklyQuest quest, BYTE index, BYTE count);
    bool Update(WeeklyQuest quest);
    WeeklyQuest* Find(const std::wstring& id);

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
