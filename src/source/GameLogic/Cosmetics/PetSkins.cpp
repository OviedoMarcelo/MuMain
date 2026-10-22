#include "stdafx.h"

#include "GameLogic/Cosmetics/PetSkins.h"

#include "Data/GameData/ItemData/ItemModelDatabase.h"
#include "Engine/AI/GOBoid.h"
#include "Engine/Object/ZzzCharacter.h"

namespace GameLogic::Cosmetics
{
namespace
{
constexpr int HelperGroup = 13;

// The helper number whose pet or mount the model type of a mount object is.
int GetHelperNumberOfMount(int mountType)
{
    switch (mountType)
    {
    case MODEL_HELPER:
        return 0;
    case MODEL_IMP:
        return 1;
    case MODEL_UNICON:
        return 2;
    case MODEL_PEGASUS:
        return 3;
    case MODEL_DARK_HORSE:
        return 4;
    case MODEL_FENRIR_BLACK:
    case MODEL_FENRIR_BLUE:
    case MODEL_FENRIR_GOLD:
    case MODEL_FENRIR_RED:
        return 37;
    default:
        return -1;
    }
}

int HelperItemType(int number)
{
    return HelperGroup * MAX_ITEM_INDEX + number;
}

CHARACTER* FindOwner(const OBJECT* mount)
{
    if (mount->Owner == nullptr)
    {
        return nullptr;
    }

    for (int i = 0; i < MAX_CHARACTERS_CLIENT; ++i)
    {
        if (&CharactersClient[i].Object == mount->Owner)
        {
            return &CharactersClient[i];
        }
    }

    return nullptr;
}

void SetSkin(OBJECT* mount, const CHARACTER* c)
{
    mount->SkinType = -1;
    const int skin = c->Helper.SkinType;
    if (skin <= 0)
    {
        return;
    }

    const int target = GetSkinTarget(skin - MODEL_ITEM);
    if (target >= 0 && target == GetHelperNumberOfMount(mount->Type))
    {
        mount->SkinType = skin;
    }
}
} // namespace

int GetSkinTarget(int itemType)
{
    if (itemType / MAX_ITEM_INDEX != HelperGroup)
    {
        return -1;
    }

    const auto* model = g_ItemModelDatabase.Find(itemType);
    return model != nullptr && model->skinOf.has_value() ? *model->skinOf : -1;
}

bool ApplyPetSkin(CHARACTER* c, int skinItemType)
{
    if (c == nullptr || c->Helper.Type <= 0)
    {
        return false;
    }

    const int target = GetSkinTarget(skinItemType);
    if (target < 0 || c->Helper.Type != MODEL_ITEM + HelperItemType(target))
    {
        return false;
    }

    c->Helper.SkinType = static_cast<short>(MODEL_ITEM + skinItemType);
    return true;
}

void ResolvePetSkin(CHARACTER* c)
{
    if (c == nullptr)
    {
        return;
    }

    c->Helper.SkinType = -1;
    if (c->Helper.Type <= 0)
    {
        return;
    }

    const int itemType = c->Helper.Type - MODEL_ITEM;
    const int target = GetSkinTarget(itemType);
    if (target >= 0)
    {
        c->Helper.SkinType = c->Helper.Type;
        c->Helper.Type = static_cast<short>(MODEL_ITEM + HelperItemType(target));
    }
}

void RefreshMountSkins(CHARACTER* c)
{
    if (c == nullptr)
    {
        return;
    }

    for (int i = 0; i < MAX_MOUNTS; ++i)
    {
        OBJECT* mount = &Mounts[i];
        if (mount->Live && mount->Owner == &c->Object)
        {
            SetSkin(mount, c);
        }
    }
}

void RefreshMountSkin(OBJECT* mount)
{
    mount->SkinType = -1;
    if (const CHARACTER* c = FindOwner(mount))
    {
        SetSkin(mount, c);
    }
}
} // namespace GameLogic::Cosmetics
