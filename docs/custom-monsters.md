# Custom Monsters

MU Cyber adds monsters which the original client doesn't know, each with a
model of its own. They have numbers above the original monsters (which go up
to 668), so they never take the place of one.

| Number | Name | Model | Texture |
|---|---|---|---|
| 700 | Coloso de Hielo | `Data/Monster/Monster218.bmd` | `Data/Monster/USK_xMonsterBoss027_01.ozt` |

The server defines what a monster does: its stats, drops and where it spawns
(the Coloso de Hielo has none: it is meant to be the boss of an encounter of the boss
encounters plugin, which spawns it and sets its health, size, glow and drops). A game
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
3. Create it in `GameLogic/Monsters/CustomMonsters.cpp`: open its model, set
   its size and look.
4. Add its name to `Data/Local/<language>/NpcName_<language>.txt`, in every
   language, before the line `end`.
5. Define it in the server with the same number.
