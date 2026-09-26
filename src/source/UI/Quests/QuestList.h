#pragma once

#include "GameLogic/Quests/WeeklyQuestCatalog.h"

#include <string>
#include <vector>

// How the quest window lists the quests: grouped by their category, and with
// a checklist of the steps of a quest.
namespace UI::Quests
{
// A row of the quest list: a quest, or the heading of a category.
struct QuestListRow
{
    static constexpr int NoQuest = -1;

    // The index of the quest in the catalog; NoQuest for a heading.
    int QuestIndex = NoQuest;
    GameLogic::Quests::QuestCategory Category = GameLogic::Quests::QuestCategory::Weekly;

    bool IsHeading() const
    {
        return QuestIndex == NoQuest;
    }
};

// Groups the quests by their category, each group with a heading, in the
// order story, daily, weekly, class, zone. Within a group, the quests keep
// the order of the server. Quests of an older server, which doesn't send
// categories, are listed without headings.
std::vector<QuestListRow> BuildListRows(const std::vector<GameLogic::Quests::WeeklyQuest>& quests);

// The quest whose reset the list shows below it: a weekly one when there is
// one, otherwise a daily one. NoQuest when all quests are done once, because
// then nothing resets.
int FindQuestOfListReset(const std::vector<GameLogic::Quests::WeeklyQuest>& quests);

// The line of a step in the checklist, e.g. "[x] Talk to the Guardian" or "[ ] Kill 50 Skeletons (12/50)".
std::wstring FormatObjectiveLine(const GameLogic::Quests::QuestObjective& objective);
} // namespace UI::Quests
