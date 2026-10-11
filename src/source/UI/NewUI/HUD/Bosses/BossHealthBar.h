#pragma once

// The bar with the name, the phase and the health of the boss of an encounter,
// at the top of the screen (see GameLogic::Monsters::BossHealth).
namespace UI::Bosses::HealthBar
{
// Draws the bar, if the server shows one.
void Render();
} // namespace UI::Bosses::HealthBar
