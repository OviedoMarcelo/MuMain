#include "stdafx.h"

#include "GameLogic/Monsters/CustomMonsters.h"

#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzOpenData.h"

#include <array>

namespace GameLogic::Monsters::Custom
{
namespace
{
// The mesh of a model which is drawn additively, so that it glows; NoGlowMesh for none.
constexpr int NoGlowMesh = -1;
// The glowing parts of the objects of the boss encounters: their runes and crystals.
constexpr int EncounterObjectGlowMesh = 1;
constexpr float GlowBrightness = 1.0f;
// The objects of the boss encounters are a bit smaller than their models, so that they fit on their tile.
constexpr float EncounterObjectScale = 0.7f;

struct CustomMonster
{
    EMonsterType Type;
    EMonsterModelType Model;
    // The size of the model; an encounter of the server can make it bigger.
    float Scale;
    int GlowMesh;
};

constexpr std::array<CustomMonster, 9> Monsters = {{
    // A giant of stone and ice.
    {MONSTER_ICE_COLOSSUS, MONSTER_MODEL_ICE_COLOSSUS, 1.0f, NoGlowMesh},
    // A demon with a great sword.
    {MONSTER_WATCHER, MONSTER_MODEL_WATCHER, 1.0f, NoGlowMesh},
    // The objects of the boss encounters, in ice and fire colors.
    {MONSTER_ICE_CRYSTAL, MONSTER_MODEL_ICE_CRYSTAL, EncounterObjectScale, EncounterObjectGlowMesh},
    {MONSTER_FIRE_CRYSTAL, MONSTER_MODEL_FIRE_CRYSTAL, EncounterObjectScale, EncounterObjectGlowMesh},
    {MONSTER_ICE_RUNE_PILLAR, MONSTER_MODEL_ICE_RUNE_PILLAR, EncounterObjectScale, EncounterObjectGlowMesh},
    {MONSTER_FIRE_RUNE_PILLAR, MONSTER_MODEL_FIRE_RUNE_PILLAR, EncounterObjectScale, EncounterObjectGlowMesh},
    {MONSTER_ICE_OBELISK, MONSTER_MODEL_ICE_OBELISK, EncounterObjectScale, EncounterObjectGlowMesh},
    {MONSTER_FIRE_OBELISK, MONSTER_MODEL_FIRE_OBELISK, EncounterObjectScale, EncounterObjectGlowMesh},
    // A winged ice queen, the strongest monster of Icarus.
    {MONSTER_ICARUS_GUARDIAN, MONSTER_MODEL_ICARUS_GUARDIAN, 1.0f, NoGlowMesh},
}};

const CustomMonster* Find(EMonsterType type)
{
    for (const auto& monster : Monsters)
    {
        if (monster.Type == type)
        {
            return &monster;
        }
    }

    return nullptr;
}
} // namespace

CHARACTER* Create(EMonsterType type, int positionX, int positionY, int key)
{
    const CustomMonster* monster = Find(type);
    if (monster == nullptr)
    {
        return nullptr;
    }

    OpenMonsterModel(monster->Model);
    CHARACTER* c = CreateCharacter(key, MODEL_MONSTER01 + monster->Model, static_cast<unsigned char>(positionX),
                                   static_cast<unsigned char>(positionY));
    c->Object.Scale = monster->Scale;
    if (monster->GlowMesh != NoGlowMesh)
    {
        c->Object.BlendMesh = monster->GlowMesh;
        c->Object.BlendMeshLight = GlowBrightness;
    }

    c->Object.LightEnable = false;
    c->Blood = false;
    return c;
}
} // namespace GameLogic::Monsters::Custom
