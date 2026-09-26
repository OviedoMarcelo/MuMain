#include "UI/Quests/QuestList.h"

#include <algorithm>
#include <array>

namespace UI::Quests
{
namespace
{
using GameLogic::Quests::QuestCategory;
using GameLogic::Quests::QuestObjective;
using GameLogic::Quests::QuestPeriod;
using GameLogic::Quests::WeeklyQuest;

// The story comes first, it's what a player follows; the repeatable quests follow.
constexpr std::array<QuestCategory, 5> CategoryOrder = {
    QuestCategory::Main, QuestCategory::Daily, QuestCategory::Weekly, QuestCategory::Class, QuestCategory::Zone,
};

constexpr const wchar_t* DoneMarker = L"[x] ";
constexpr const wchar_t* OpenMarker = L"[  ] ";

// Without details, the server doesn't know categories and every quest is a weekly one,
// so a heading wouldn't tell anything.
bool KnowsCategories(const std::vector<WeeklyQuest>& quests)
{
    return std::any_of(quests.begin(), quests.end(), [](const WeeklyQuest& quest) { return quest.HasDetails; });
}

void AddGroup(const std::vector<WeeklyQuest>& quests, QuestCategory category, std::vector<QuestListRow>& rows)
{
    const auto isInGroup = [category](const WeeklyQuest& quest) { return quest.Category == category; };
    if (std::none_of(quests.begin(), quests.end(), isInGroup))
    {
        return;
    }

    rows.push_back({QuestListRow::NoQuest, category});
    for (size_t i = 0; i < quests.size(); ++i)
    {
        if (isInGroup(quests[i]))
        {
            rows.push_back({static_cast<int>(i), category});
        }
    }
}
} // namespace

std::vector<QuestListRow> BuildListRows(const std::vector<WeeklyQuest>& quests)
{
    std::vector<QuestListRow> rows;
    if (!KnowsCategories(quests))
    {
        rows.reserve(quests.size());
        for (size_t i = 0; i < quests.size(); ++i)
        {
            rows.push_back({static_cast<int>(i), quests[i].Category});
        }

        return rows;
    }

    rows.reserve(quests.size() + CategoryOrder.size());
    for (const auto category : CategoryOrder)
    {
        AddGroup(quests, category, rows);
    }

    return rows;
}

int FindQuestOfListReset(const std::vector<WeeklyQuest>& quests)
{
    // Without details (an older server), every quest is a weekly one.
    const auto isWeekly = [](const WeeklyQuest& quest) { return !quest.HasDetails || quest.Period == QuestPeriod::Weekly; };
    const auto isDaily = [](const WeeklyQuest& quest) { return quest.HasDetails && quest.Period == QuestPeriod::Daily; };

    auto found = std::find_if(quests.begin(), quests.end(), isWeekly);
    if (found == quests.end())
    {
        found = std::find_if(quests.begin(), quests.end(), isDaily);
    }

    return found == quests.end() ? QuestListRow::NoQuest : static_cast<int>(found - quests.begin());
}

std::wstring FormatObjectiveLine(const QuestObjective& objective)
{
    std::wstring line = objective.IsDone ? DoneMarker : OpenMarker;
    line += objective.Text;

    // A step which is done once, like talking to an NPC, doesn't need a counter.
    const bool showsCount = !objective.IsDone && objective.RequiredCount > 1;
    if (showsCount)
    {
        line += L" (" + std::to_wstring(objective.CurrentCount) + L"/" + std::to_wstring(objective.RequiredCount) + L")";
    }

    return line;
}
} // namespace UI::Quests
