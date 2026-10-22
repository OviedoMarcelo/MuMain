#pragma once

// The shapes which the quest tracker draws: its frame, the markers of the
// quests, the icons and the progress bar. They are drawn with colors only, so
// that the tracker doesn't need textures. The positions and sizes are in
// coordinates of the reference resolution, like the rest of the UI.
namespace UI::Quests::TrackerShapes
{
// A panel with cut corners, filled and with a border of one pixel.
void DrawPanel(float x, float y, float width, float height, float corner, unsigned int fill, unsigned int border);

// A diamond around the center. The fill blends from the top to the bottom.
void DrawDiamond(float centerX, float centerY, float radius, unsigned int border, unsigned int fillTop,
                 unsigned int fillBottom);

// The marker of the quest which is open: a gold diamond with a dark ring.
void DrawOpenQuestMarker(float centerX, float centerY, float radius);

// The marker of a folded quest: a dark diamond with a gold border.
void DrawFoldedQuestMarker(float centerX, float centerY, float radius);

// An arrow head which points up or down.
void DrawChevron(float centerX, float centerY, float size, bool pointsUp, unsigned int color);

// The treasure chest in front of the rewards, with its top left corner at x, y.
void DrawChest(float x, float y);

// A bar which is filled from the left by the ratio, from 0 to 1.
void DrawProgressBar(float x, float y, float width, float height, float ratio);

// The width and the height of the chest.
constexpr float ChestWidth = 12.0f;
constexpr float ChestHeight = 10.0f;
} // namespace UI::Quests::TrackerShapes
