#pragma once

#include "Core/Globals/_enum.h"

class CHARACTER;

// The monsters of MU Cyber which the original client doesn't know, each with a
// model of its own. They have numbers above the original ones, which go up to 668,
// and their names are in Data/Local/<language>/NpcName_<language>.txt like the
// others. The server defines their stats.
namespace GameLogic::Monsters::Custom
{
// Creates the monster in the viewport, when the type is one of ours.
// Returns nullptr for the other types, which are created as usual.
CHARACTER* Create(EMonsterType type, int positionX, int positionY, int key);
} // namespace GameLogic::Monsters::Custom
