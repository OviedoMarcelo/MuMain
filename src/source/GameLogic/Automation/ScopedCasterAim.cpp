#include "stdafx.h"
#include "GameLogic/Automation/ScopedCasterAim.h"

#include "GameLogic/Combat/CombatTarget.h"
#include "Engine/Object/ZzzCharacter.h"

// Defined in ZzzInterface.cpp.
extern int SelectedCharacter;

namespace GameLogic::Automation
{
ScopedCasterAim::ScopedCasterAim(const CHARACTER* caster)
    : m_previousSelection(SelectedCharacter)
{
    SelectedCharacter = -1;
    GameLogic::Combat::SetCursorFreeAim(caster->Object.Position);
}

ScopedCasterAim::~ScopedCasterAim()
{
    GameLogic::Combat::SetCursorFreeAim(nullptr);
    SelectedCharacter = m_previousSelection;
}
} // namespace GameLogic::Automation
