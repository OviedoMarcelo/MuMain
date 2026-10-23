#pragma once

// Calculations with the ARGB colors of the shapes which the quest windows draw
// (0xAARRGGBB, like RenderColorQuadARGB takes them).
namespace UI::Quests::ColorMath
{
// The color with its alpha multiplied by the opacity, from 0 to 1.
unsigned int Fade(unsigned int color, float opacity);

// The color with this alpha, from 0 to 1, instead of its own.
unsigned int WithAlpha(unsigned int color, float alpha);

// Mixes two colors, all four channels: 0 is the first one, 1 the second one.
unsigned int Blend(unsigned int from, unsigned int to, float ratio);
} // namespace UI::Quests::ColorMath
