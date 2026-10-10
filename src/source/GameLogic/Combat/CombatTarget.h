#pragma once

class CHARACTER;

// Resolves the current attack target tile from the selection (or the ground
// cursor) and records it on the character. Extracted from ZzzInterface.cpp.
namespace GameLogic::Combat
{
    bool CheckTarget(CHARACTER* c);

    // While a position is set, CheckTarget aims a cast with no selected
    // character at it instead of at the ground under the cursor. Automated
    // casts set it so a skill never aims, or walks, wherever the mouse
    // happens to rest. nullptr clears it.
    void SetCursorFreeAim(const float* position);
}
