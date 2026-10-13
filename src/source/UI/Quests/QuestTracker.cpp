#include "UI/Quests/QuestTracker.h"

#include <algorithm>

namespace UI::Quests
{
namespace
{
using GameLogic::Quests::QuestObjective;
using GameLogic::Quests::QuestPeriod;
using GameLogic::Quests::WeeklyQuest;

bool IsStory(const WeeklyQuest& quest)
{
    return quest.HasDetails && quest.Period == QuestPeriod::Once;
}

bool HasProgress(const WeeklyQuest& quest)
{
    return quest.CurrentCount > 0 ||
           std::any_of(quest.Objectives.begin(), quest.Objectives.end(),
                       [](const QuestObjective& objective) { return objective.CurrentCount > 0 || objective.IsDone; });
}

bool IsInProgress(const WeeklyQuest& quest)
{
    if (quest.IsRewarded)
    {
        return false;
    }

    return quest.IsCompleted || IsStory(quest) || HasProgress(quest);
}

std::wstring FormatCount(uint32_t current, uint32_t required)
{
    return std::to_wstring((std::min)(current, required)) + L"/" + std::to_wstring(required);
}

// The objective to do now: the current step of a sequential quest, otherwise the first which isn't done.
const QuestObjective* FindCurrentObjective(const WeeklyQuest& quest)
{
    if (quest.IsSequential && quest.CurrentStep < quest.Objectives.size())
    {
        return &quest.Objectives[quest.CurrentStep];
    }

    const auto open = std::find_if(quest.Objectives.begin(), quest.Objectives.end(),
                                   [](const QuestObjective& objective) { return !objective.IsDone; });
    return open == quest.Objectives.end() ? nullptr : &*open;
}

std::wstring GetStep(const WeeklyQuest& quest)
{
    if (quest.IsCompleted)
    {
        return {};
    }

    const auto* objective = FindCurrentObjective(quest);
    if (objective == nullptr)
    {
        // Without details from the server we only know the count of the quest.
        return quest.RequiredCount > 1 ? FormatCount(quest.CurrentCount, quest.RequiredCount) : std::wstring();
    }

    // A step which is just done once, e.g. talking to an NPC, doesn't need a count.
    if (objective->RequiredCount <= 1)
    {
        return objective->Text;
    }

    const auto count = FormatCount(objective->CurrentCount, objective->RequiredCount);
    return objective->Text.empty() ? count : objective->Text + L" " + count;
}

void Add(const WeeklyQuest& quest, QuestTrackerContent& content)
{
    content.Quests.push_back({quest.Name, GetStep(quest), quest.IsRewardPending()});
    if (content.Chapter.empty() && IsStory(quest))
    {
        content.Chapter = quest.Chapter;
    }
}
} // namespace

QuestTrackerContent BuildQuestTracker(const std::vector<WeeklyQuest>& quests, size_t maxQuests)
{
    QuestTrackerContent content;
    for (const auto& quest : quests)
    {
        if (content.Quests.size() < maxQuests && IsStory(quest) && IsInProgress(quest))
        {
            Add(quest, content);
        }
    }

    for (const auto& quest : quests)
    {
        if (content.Quests.size() < maxQuests && !IsStory(quest) && IsInProgress(quest))
        {
            Add(quest, content);
        }
    }

    return content;
}
} // namespace UI::Quests
