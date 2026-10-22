#pragma once

#include "GameLogic/Quests/WeeklyQuestCatalog.h"

namespace UI::Quests
{
// The type of a quest in singular, e.g. "Story" or "Weekly", in the language of the client.
const wchar_t* GetTypeName(GameLogic::Quests::QuestCategory category);
} // namespace UI::Quests
