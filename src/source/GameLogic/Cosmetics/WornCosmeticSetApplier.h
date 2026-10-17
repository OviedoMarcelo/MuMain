#pragma once

class CHARACTER;

namespace GameLogic::Cosmetics
{
// Shows the pieces of the cosmetic set which the own character wears instead of
// its armor. The other players see the set through the appearance data which
// the server sends, but the own character is built from its inventory, so it's
// applied here each time the armor of the hero is set. Pieces which the game
// doesn't have, e.g. the helm of some sets, keep the real armor.
void ApplyWornSet(CHARACTER* hero);
} // namespace GameLogic::Cosmetics
