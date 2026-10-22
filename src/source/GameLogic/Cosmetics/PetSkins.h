#pragma once

class CHARACTER;
class OBJECT;

namespace GameLogic::Cosmetics
{
// Skins of pets and mounts: a model (Data/Items/Models, group 13) with
// "skinOf" replaces the look of one pet or mount, e.g. of the Guardian Angel.
// The character keeps its real helper, so the pet still moves and does what
// it does; only its model is drawn instead (OBJECT::SkinType).

// The helper number whose pet or mount the item type is a skin of; -1 if the
// item isn't a skin.
int GetSkinTarget(int itemType);

// Makes the helper part show the item of a cosmetic piece, if it's a skin of
// the helper the part has: the part keeps its type and gets the skin as
// SkinType. Returns false if the item isn't a skin of that helper.
bool ApplyPetSkin(CHARACTER* c, int skinItemType);

// The helper part of the other players comes with the skin as its item: the
// part gets the helper the skin is of as its type, and the skin as SkinType.
void ResolvePetSkin(CHARACTER* c);

// Sets the model of the mounts and pets of the character from the SkinType of
// its helper part. Called when they are created and when the skin changes.
void RefreshMountSkins(CHARACTER* c);

// The same for the owner of a mount which has just been created.
void RefreshMountSkin(OBJECT* mount);
} // namespace GameLogic::Cosmetics
