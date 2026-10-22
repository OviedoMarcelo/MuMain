#pragma once

#include "GameLogic/Quests/WeeklyQuestCatalog.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// What the tracker at the top right of the screen shows: the chapter of the
// story and the quests in progress, so that a player sees what to do next
// without opening the quest window.
namespace UI::Quests
{
struct TrackedQuest
{
    std::wstring Id;
    std::wstring Name;
    GameLogic::Quests::QuestCategory Category = GameLogic::Quests::QuestCategory::Weekly;
    // What to do now, e.g. "Kill 50 Skeletons", or the description when the server sends no steps;
    // empty when the quest waits for its reward.
    std::wstring Objective;
    // The progress of what to do now; ObjectiveRequired is 0 when there is nothing to count.
    uint32_t ObjectiveCurrent = 0;
    uint32_t ObjectiveRequired = 0;
    // The progress of the whole quest: its count, or the done steps of a quest with several.
    uint32_t QuestCurrent = 0;
    uint32_t QuestRequired = 0;
    std::wstring Rewards;
    // The quest is completed, but its rewards wait for free inventory space.
    bool IsRewardPending = false;
};

struct QuestTrackerContent
{
    // The chapter of the first story quest in progress; empty without one.
    std::wstring Chapter;
    std::vector<TrackedQuest> Quests;

    bool IsEmpty() const
    {
        return Quests.empty();
    }
};

// Picks the quests which aren't done yet: the story first, then the quests which
// made progress or wait for their reward, then the others. Within each group
// the quests keep the order of the server. At most maxQuests.
QuestTrackerContent BuildQuestTracker(const std::vector<GameLogic::Quests::WeeklyQuest>& quests, size_t maxQuests);
} // namespace UI::Quests
