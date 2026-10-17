#include "stdafx.h"

#include "GameLogic/Cosmetics/WornCosmeticSetApplier.h"

#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInfomation.h"
#include "GameLogic/Cosmetics/CosmeticSetCatalog.h"

namespace GameLogic::Cosmetics
{
namespace
{
// The item group of the helms. The groups of armors, pants, gloves and boots
// follow it, in the same order as their body parts.
constexpr int HelmGroup = 7;

bool HasItem(int itemIndex)
{
    return ItemAttribute[itemIndex].Name[0] != L'\0';
}

void ApplyPiece(PART_t& part, int group, const CosmeticSet& set)
{
    const int itemIndex = static_cast<int>(ITEMINDEX(group, set.ItemNumber));
    if (!HasItem(itemIndex))
    {
        return;
    }

    part.Type = itemIndex + MODEL_ITEM;
    part.Level = set.Level;
    part.ExcellentFlags = set.IsExcellent ? 1 : 0;
    part.AncientDiscriminator = 0;
}
} // namespace

void ApplyWornSet(CHARACTER* hero)
{
    if (hero == nullptr)
    {
        return;
    }

    const CosmeticSet* set = CosmeticSets().GetWornSet();
    if (set == nullptr)
    {
        return;
    }

    for (int bodyPart = BODYPART_HELM; bodyPart <= BODYPART_BOOTS; ++bodyPart)
    {
        ApplyPiece(hero->BodyPart[bodyPart], HelmGroup + bodyPart - BODYPART_HELM, *set);
    }
}
} // namespace GameLogic::Cosmetics
