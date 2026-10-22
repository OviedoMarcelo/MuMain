#include "stdafx.h"

#include "UI/NewUI/Quests/QuestTrackerShapes.h"

#include "Render/Textures/ZzzOpenglUtil.h"

#include <algorithm>

namespace UI::Quests::TrackerShapes
{
namespace
{
constexpr float LineThickness = 1.0f;
constexpr float ChevronThickness = 1.5f;

constexpr unsigned int GoldLight = 0xFFFFE6A0u;
constexpr unsigned int Gold = 0xFFD9A441u;
constexpr unsigned int GoldDark = 0xFF9A6A26u;
constexpr unsigned int GoldBorder = 0xFFB98A45u;
constexpr unsigned int MarkerRing = 0xFF1A140Cu;
constexpr unsigned int FoldedMarkerFill = 0xFF1C2628u;
constexpr unsigned int FoldedMarkerBorder = 0xFF8C6C3Cu;

// The chest: a wooden body and lid with gold fittings.
constexpr unsigned int ChestWood = 0xFF6B4423u;
constexpr unsigned int ChestLid = 0xFF8D5C30u;
constexpr unsigned int ChestFitting = 0xFFD9A441u;
constexpr unsigned int ChestOutline = 0xFF3A2410u;
constexpr float ChestLidHeight = 4.0f;
constexpr float ChestLidSlope = 1.0f;
constexpr float ChestBandWidth = 2.0f;
constexpr float ChestLockSize = 2.0f;

constexpr unsigned int BarBackground = 0xFF0B0B0Bu;
constexpr unsigned int BarBorder = 0xFF55504Au;
constexpr unsigned int BarFillStart = 0xFF9A6420u;
constexpr unsigned int BarFillEnd = 0xFFF0C25Au;
constexpr float BarCorner = 2.0f;

// The ring of the open quest marker and its gold center, relative to the radius.
constexpr float MarkerRingScale = 0.72f;
constexpr float MarkerCenterScale = 0.48f;

void DrawLine(float x1, float y1, float x2, float y2, unsigned int color)
{
    RenderColorLineARGB(x1, y1, x2, y2, LineThickness, color);
}

void DrawRect(float x, float y, float width, float height, unsigned int color)
{
    RenderColorQuadARGB(x, y, width, height, color);
}
} // namespace

void DrawPanel(float x, float y, float width, float height, float corner, unsigned int fill, unsigned int border)
{
    const float right = x + width;
    const float bottom = y + height;

    const float leftX[4] = {x, x + corner, x + corner, x};
    const float leftY[4] = {y + corner, y, bottom, bottom - corner};
    const float rightX[4] = {right - corner, right, right, right - corner};
    const float rightY[4] = {y, y + corner, bottom - corner, bottom};
    const unsigned int colors[4] = {fill, fill, fill, fill};
    RenderColorQuadPointsARGB(leftX, leftY, colors);
    DrawRect(x + corner, y, width - 2 * corner, height, fill);
    RenderColorQuadPointsARGB(rightX, rightY, colors);

    DrawLine(x + corner, y, right - corner, y, border);
    DrawLine(right - corner, y, right, y + corner, border);
    DrawLine(right, y + corner, right, bottom - corner, border);
    DrawLine(right, bottom - corner, right - corner, bottom, border);
    DrawLine(right - corner, bottom, x + corner, bottom, border);
    DrawLine(x + corner, bottom, x, bottom - corner, border);
    DrawLine(x, bottom - corner, x, y + corner, border);
    DrawLine(x, y + corner, x + corner, y, border);
}

void DrawDiamond(float centerX, float centerY, float radius, unsigned int border, unsigned int fillTop,
                 unsigned int fillBottom)
{
    const float x[4] = {centerX, centerX + radius, centerX, centerX - radius};
    const float y[4] = {centerY - radius, centerY, centerY + radius, centerY};
    const unsigned int colors[4] = {fillTop, fillBottom, fillBottom, fillTop};
    RenderColorQuadPointsARGB(x, y, colors);

    for (int corner = 0; corner < 4; ++corner)
    {
        const int next = (corner + 1) % 4;
        DrawLine(x[corner], y[corner], x[next], y[next], border);
    }
}

void DrawOpenQuestMarker(float centerX, float centerY, float radius)
{
    DrawDiamond(centerX, centerY, radius, GoldBorder, Gold, GoldDark);
    DrawDiamond(centerX, centerY, radius * MarkerRingScale, MarkerRing, MarkerRing, MarkerRing);
    DrawDiamond(centerX, centerY, radius * MarkerCenterScale, Gold, GoldLight, Gold);
}

void DrawFoldedQuestMarker(float centerX, float centerY, float radius)
{
    DrawDiamond(centerX, centerY, radius, FoldedMarkerBorder, FoldedMarkerFill, FoldedMarkerFill);
}

void DrawChevron(float centerX, float centerY, float size, bool pointsUp, unsigned int color)
{
    const float tipY = pointsUp ? centerY - size / 2 : centerY + size / 2;
    const float endY = pointsUp ? centerY + size / 2 : centerY - size / 2;
    RenderColorLineARGB(centerX - size, endY, centerX, tipY, ChevronThickness, color);
    RenderColorLineARGB(centerX, tipY, centerX + size, endY, ChevronThickness, color);
}

void DrawChest(float x, float y)
{
    const float right = x + ChestWidth;
    const float bottom = y + ChestHeight;
    const float lidBottom = y + ChestLidHeight;

    const float lidX[4] = {x, x + ChestLidSlope, right - ChestLidSlope, right};
    const float lidY[4] = {lidBottom, y, y, lidBottom};
    const unsigned int lidColors[4] = {ChestWood, ChestLid, ChestLid, ChestWood};
    RenderColorQuadPointsARGB(lidX, lidY, lidColors);
    DrawRect(x, lidBottom, ChestWidth, ChestHeight - ChestLidHeight, ChestWood);

    const float bandX = x + (ChestWidth - ChestBandWidth) / 2;
    DrawRect(bandX, y, ChestBandWidth, ChestHeight, ChestFitting);
    DrawLine(x, lidBottom, right, lidBottom, ChestFitting);
    DrawRect(x + (ChestWidth - ChestLockSize) / 2, lidBottom, ChestLockSize, ChestLockSize, ChestOutline);

    DrawLine(x, lidBottom, x, bottom, ChestOutline);
    DrawLine(x, bottom, right, bottom, ChestOutline);
    DrawLine(right, bottom, right, lidBottom, ChestOutline);
}

void DrawProgressBar(float x, float y, float width, float height, float ratio)
{
    DrawPanel(x, y, width, height, BarCorner, BarBackground, BarBorder);

    const float filled = (width - 2 * LineThickness) * (std::clamp)(ratio, 0.0f, 1.0f);
    if (filled <= 0.0f)
    {
        return;
    }

    const float left = x + LineThickness;
    const float top = y + LineThickness;
    const float bottom = y + height - LineThickness;
    const float fillX[4] = {left, left + filled, left + filled, left};
    const float fillY[4] = {top, top, bottom, bottom};
    const unsigned int colors[4] = {BarFillStart, BarFillEnd, BarFillEnd, BarFillStart};
    RenderColorQuadPointsARGB(fillX, fillY, colors);
}
} // namespace UI::Quests::TrackerShapes
