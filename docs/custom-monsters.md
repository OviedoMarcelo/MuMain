# Custom Monsters

MU Cyber adds monsters which the original client doesn't know, each with a
model of its own. They have numbers above the original monsters (which go up
to 668), so they never take the place of one.

| Number | Name | Model | Texture |
|---|---|---|---|
| 700 | Coloso de Hielo | `Data/Monster/Monster218.bmd` | `Data/Monster/USK_xMonsterBoss027_01.ozt` |
| 701 | El Vigía | `Data/Monster/Monster219.bmd` | `Data/Monster/USK_xMonsterBoss003_01.ozt` |
| 702, 703 | Cristal de Hielo, Cristal de Fuego | `Monster220.bmd`, `Monster221.bmd` | `CyberEncounterStone*.ozt`, `CyberEncounterGlow*.ozt` |
| 704, 705 | Pilar Rúnico de Hielo, de Fuego | `Monster222.bmd`, `Monster223.bmd` | the same |
| 706, 707 | Obelisco de Hielo, de Fuego | `Monster224.bmd`, `Monster225.bmd` | the same |

The crystals, pillars and obelisks are objects for the phases of the boss
encounters (like the pillars and shields which the players have to break). They
don't move nor attack, and their crystals float and turn; when they're
destroyed, they sink into the ground. Their runes and crystals glow, and
little flames rise from them (orange from the fire ones, blue with sparkles
from the ice ones, see `Render/Monsters/EncounterObjectParticles.cpp`). They are
made by `tools/models/make_encounter_objects.js` (`node` runs it), which
writes them again, e.g. after changing their shape or colors.

The server defines what a monster does: its stats, drops and where it spawns
(these have none: they are meant to be the bosses of encounters of the boss encounters
plugin, which spawns them and sets their health, size, glow and drops). A game
master can call one next to them with `/createmonster <number>`.

## Adding one

1. Copy the model to `Data/Monster/Monster<N>.bmd`, where `N` is the number of
   its model type plus one, and its textures next to it with the names which
   the model asks for. A `.tga` in the model is read from the `.ozt` file, a
   `.jpg` from the `.ozj` file. The model needs the 7 animations of a monster:
   two of standing, walking, two attacks, being hit and dying.
2. In `Core/Globals/_enum.h`, add its model type before `MONSTER_MODEL_COUNT`
   (with its `MODEL_...` alias), and its monster number to `EMonsterType`. Add
   the name of the model type to the list of `GetMonsterModelName` in
   `ZzzOpenData.cpp`.
3. Add it to the table of `GameLogic/Monsters/CustomMonsters.cpp`, with its
   model type, size and the mesh which glows, if any.
4. Add its name to `Data/Local/<language>/NpcName_<language>.txt`, in every
   language, before the line `end`.
5. Define it in the server with the same number.
