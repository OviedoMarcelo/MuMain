#pragma once

class CHARACTER;

namespace GameLogic::Cosmetics
{
// Shows the cosmetic pieces which the own character wears instead of the items
// of their slots. The other players see them through the appearance data which
// the server sends, but the own character is built from its inventory, so they
// are applied here each time the armor of the hero is set. Pieces which the game
// doesn't have keep the real item.
void ApplyWornPieces(CHARACTER* hero);
} // namespace GameLogic::Cosmetics
