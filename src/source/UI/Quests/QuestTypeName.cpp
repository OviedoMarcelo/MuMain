#include "stdafx.h"
#include "I18N/All.h"

#include "UI/Quests/QuestTypeName.h"

namespace UI::Quests
{
const wchar_t* GetTypeName(GameLogic::Quests::QuestCategory category)
{
    using GameLogic::Quests::QuestCategory;
    switch (category)
    {
    case QuestCategory::Main:
        return I18N::Game::QuestTypeMain;
    case QuestCategory::Daily:
        return I18N::Game::QuestTypeDaily;
    case QuestCategory::Class:
        return I18N::Game::QuestTypeClass;
    case QuestCategory::Zone:
        return I18N::Game::QuestTypeZone;
    default:
        return I18N::Game::QuestTypeWeekly;
    }
}
} // namespace UI::Quests
