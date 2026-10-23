#include "stdafx.h"

#include "GameLogic/Monsters/CustomMonsters.h"

#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzOpenData.h"

#include <array>

namespace GameLogic::Monsters::Custom
{
namespace
{
struct CustomMonster
{
    EMonsterType Type;
    EMonsterModelType Model;
    // The size of the model; an encounter of the server can make it bigger.
    float Scale;
};

constexpr std::array<CustomMonster, 2> Monsters = {{
    // A giant of stone and ice.
    {MONSTER_ICE_COLOSSUS, MONSTER_MODEL_ICE_COLOSSUS, 1.0f},
    // A demon with a great sword.
    {MONSTER_WATCHER, MONSTER_MODEL_WATCHER, 1.0f},
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
    c->Object.LightEnable = false;
    c->Blood = false;
    return c;
}
} // namespace GameLogic::Monsters::Custom
