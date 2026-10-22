#include "stdafx.h"

#include "GameLogic/Cosmetics/WornCosmeticsApplier.h"

#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInfomation.h"
#include "GameLogic/Cosmetics/CosmeticPieceCatalog.h"
#include "GameLogic/Cosmetics/PetSkins.h"

namespace GameLogic::Cosmetics
{
namespace
{
// The item group of the helms. The groups of armors, pants, gloves and boots
// follow it, in the same order as their body parts.
constexpr int HelmGroup = 7;
constexpr int WingsGroup = 12;
constexpr int HelperGroup = 13;

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

void ApplyArmorPieces(CHARACTER* hero)
{
    for (int bodyPart = BODYPART_HELM; bodyPart <= BODYPART_BOOTS; ++bodyPart)
    {
        const auto group = static_cast<BYTE>(HelmGroup + bodyPart - BODYPART_HELM);
        if (const CosmeticPiece* piece = CosmeticPieces().GetWornPiece(group))
        {
            ApplyPiece(hero->BodyPart[bodyPart], *piece);
        }
    }
}

// The wings are shown like the armor, also without real wings.
void ApplyWingsPiece(CHARACTER* hero)
{
    if (const CosmeticPiece* piece = CosmeticPieces().GetWornPiece(WingsGroup))
    {
        ApplyPiece(hero->Wing, *piece);
    }
}

// A pet skin is only shown over the pet or mount it's a skin of.
void ApplyPetPiece(CHARACTER* hero)
{
    hero->Helper.SkinType = -1;
    if (const CosmeticPiece* piece = CosmeticPieces().GetWornPiece(HelperGroup))
    {
        ApplyPetSkin(hero, static_cast<int>(ITEMINDEX(piece->ItemGroup, piece->ItemNumber)));
    }

    RefreshMountSkins(hero);
}
} // namespace

void ApplyWornPieces(CHARACTER* hero)
{
    if (hero == nullptr)
    {
        return;
    }

    ApplyArmorPieces(hero);
    ApplyWingsPiece(hero);
    ApplyPetPiece(hero);
}
} // namespace GameLogic::Cosmetics
