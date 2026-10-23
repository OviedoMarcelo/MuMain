#include "stdafx.h"

#include "GameLogic/Monsters/CustomMonsters.h"

#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzOpenData.h"

namespace GameLogic::Monsters::Custom
{
namespace
{
// The Coloso de Hielo: a giant of stone and ice, the boss of an encounter. The encounter can make it bigger.
constexpr float IceColossusScale = 1.0f;

CHARACTER* CreateIceColossus(int positionX, int positionY, int key)
{
    OpenMonsterModel(MONSTER_MODEL_ICE_COLOSSUS);
    CHARACTER* c = CreateCharacter(key, MODEL_ICE_COLOSSUS, static_cast<unsigned char>(positionX),
                                   static_cast<unsigned char>(positionY));
    c->Object.Scale = IceColossusScale;
    c->Object.LightEnable = false;
    c->Blood = false;
    return c;
}
} // namespace

CHARACTER* Create(EMonsterType type, int positionX, int positionY, int key)
{
    switch (type)
    {
    case MONSTER_ICE_COLOSSUS:
        return CreateIceColossus(positionX, positionY, key);
    default:
        return nullptr;
    }
}
} // namespace GameLogic::Monsters::Custom
