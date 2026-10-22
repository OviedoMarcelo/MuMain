#pragma once

#include <array>

namespace UI::Quests
{
// Little golden lights which rise slowly over a square area, e.g. the portrait
// of an NPC, swaying a bit and fading in and out. Their place only depends on
// the time, so there's nothing to update between the frames.
class GoldenMotes
{
public:
    GoldenMotes();

    // Draws the motes over the square at x, y, after the seconds since they started.
    // The opacity (0 to 1) fades all of them, e.g. while their window opens.
    void Render(float x, float y, float size, float seconds, float opacity) const;

private:
    struct Mote
    {
        // Where it rises, from 0 (left) to 1 (right).
        float Column = 0.0f;
        // How many times per second it rises from the bottom to the top.
        float RisesPerSecond = 0.0f;
        // Where in its rise it starts, from 0 to 1, so that they don't rise together.
        float Phase = 0.0f;
        // How far it sways to the sides, relative to the size of the area.
        float Sway = 0.0f;
        // The radius of its bright center.
        float Radius = 0.0f;
    };

    static constexpr int MoteCount = 8;

    void RenderMote(const Mote& mote, float x, float y, float size, float seconds, float opacity) const;

    std::array<Mote, MoteCount> m_motes;
};
} // namespace UI::Quests
