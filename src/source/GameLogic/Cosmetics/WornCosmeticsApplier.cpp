#include "stdafx.h"

#include "GameLogic/Cosmetics/WornCosmeticsApplier.h"

#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInfomation.h"
#include "GameLogic/Cosmetics/CosmeticPieceCatalog.h"

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

void ApplyPiece(PART_t& part, const CosmeticPiece& piece)
{
    const int itemIndex = static_cast<int>(ITEMINDEX(piece.ItemGroup, piece.ItemNumber));
    if (!HasItem(itemIndex))
    {
        return;
    }

    part.Type = itemIndex + MODEL_ITEM;
    part.Level = piece.Level;
    part.ExcellentFlags = piece.IsExcellent ? 1 : 0;
    part.AncientDiscriminator = 0;
}
} // namespace

void ApplyWornPieces(CHARACTER* hero)
{
    if (hero == nullptr)
    {
        return;
    }

    for (int bodyPart = BODYPART_HELM; bodyPart <= BODYPART_BOOTS; ++bodyPart)
    {
        const auto group = static_cast<BYTE>(HelmGroup + bodyPart - BODYPART_HELM);
        if (const CosmeticPiece* piece = CosmeticPieces().GetWornPiece(group))
        {
            ApplyPiece(hero->BodyPart[bodyPart], *piece);
        }
    }
}
} // namespace GameLogic::Cosmetics
