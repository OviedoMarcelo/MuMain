#pragma once

#include "GameLogic/Monsters/MonsterAppearance.h"

namespace GameLogic::Monsters
{
// Shows the monster of the appearance with its size, glow and name. The
// monster has to be in view already, as the server sends the appearance right
// after it. Returns false when there is no such monster.
bool ApplyMonsterAppearance(const MonsterAppearance& appearance);
} // namespace GameLogic::Monsters
