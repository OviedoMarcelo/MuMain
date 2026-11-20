#include "stdafx.h"

#include "CyberShopStyle.h"

#include "Render/Textures/ZzzOpenglUtil.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/Dialogs/NewUICommonMessageBox.h"
#include "UI/NewUI/Quests/QuestTrackerShapes.h"

namespace GameShop::CyberStyle
{
namespace
{
namespace Shapes = UI::Quests::TrackerShapes;

constexpr unsigned int DisabledFill = 0xFF141016u;
constexpr unsigned int DisabledBorder = 0xFF3A3236u;
constexpr TextColor DisabledText = {110, 100, 112};
} // namespace

void RenderTextAt(int x, int y, const wchar_t* text, int width, TextColor color, bool isBold, int sort)
{
    g_pRenderText->SetFont(isBold ? g_hFontBold : g_hFont);
    g_pRenderText->SetTextColor(color.Red, color.Green, color.Blue, 255);
    g_pRenderText->RenderText(x, y, text, width, 0, sort);
}

int MeasureText(const wchar_t* text)
{
    return g_pRenderText->MeasureText(text, static_cast<int>(wcslen(text))).cx;
}

void DrawActionButton(float x, float y, float width, float height, bool isHovered)
{
    Shapes::DrawPanel(x, y, width, height, SmallCorner, isHovered ? ActionHoverFill : ActionFill,
                      isHovered ? ActionHoverBorder : ActionBorder);
}

void DrawSecondaryButton(float x, float y, float width, float height, bool isHovered)
{
    Shapes::DrawPanel(x, y, width, height, SmallCorner, isHovered ? TabHoverFill : TabFill,
                      isHovered ? ActiveBorder : TabBorder);
}

void DrawInset(float x, float y, float width, float height)
{
    Shapes::DrawPanel(x, y, width, height, SmallCorner, ListFill, ListBorder);
}

void DrawCoin(float centerX, float centerY, float radius)
{
    Shapes::DrawDiamond(centerX, centerY, radius, CoinBorder, CoinTop, CoinBottom);
}

void DrawMessageBox(float x, float y, float width, float height, const wchar_t* title)
{
    const float right = x + width;
    const float bottom = y + height;

    Shapes::DrawPanel(x, y, width, height, WindowCorner, WindowFill, WindowBorder);

    // The faint grid of the website.
    for (float lineX = x + GridStep; lineX < right; lineX += GridStep)
    {
        RenderColorLineARGB(lineX, y + 1, lineX, bottom - 1, 1.0f, GridLine);
    }
    for (float lineY = y + GridStep; lineY < bottom; lineY += GridStep)
    {
        RenderColorLineARGB(x + 1, lineY, right - 1, lineY, 1.0f, GridLine);
    }

    // The line under the title, with a diamond at each end.
    const float lineY = y + MessageBoxTitleHeight;
    const float lineLeft = x + 10;
    const float lineRight = right - 10;
    RenderColorLineARGB(lineLeft, lineY, lineRight, lineY, 1.0f, HeaderLine);
    Shapes::DrawDiamond(lineLeft, lineY, 3.0f, WindowBorder, WindowFill, WindowFill);
    Shapes::DrawDiamond(lineRight, lineY, 3.0f, WindowBorder, WindowFill, WindowFill);
    EndRenderColor();

    if (title != nullptr)
    {
        g_pRenderText->SetBgColor(0, 0, 0, 0);
        RenderTextAt(static_cast<int>(x), static_cast<int>(y) + 10, title, static_cast<int>(width), GoldText, true,
                     RT3_SORT_CENTER);
    }
}

void RenderMessageBoxButton(SEASON3B::CNewUIMessageBoxButton& button, bool isPrimary)
{
    const float x = button.GetPosX();
    const float y = button.GetPosY();
    const float width = button.GetWidth();
    const float height = button.GetHeight();
    const bool isEnabled = button.IsEnabled();
    const bool isHovered = isEnabled && button.GetEventState() != SEASON3B::CNewUIMessageBoxButton::EVENT_NONE;

    // The textures of the buttons had empty space around them, so the buttons are drawn a little smaller.
    const float drawX = x + 2;
    const float drawY = y + 3;
    const float drawWidth = width - 4;
    const float drawHeight = height - 6;

    TextColor textColor = WhiteText;
    if (!isEnabled)
    {
        Shapes::DrawPanel(drawX, drawY, drawWidth, drawHeight, SmallCorner, DisabledFill, DisabledBorder);
        textColor = DisabledText;
    }
    else if (isPrimary)
    {
        DrawActionButton(drawX, drawY, drawWidth, drawHeight, isHovered);
        textColor = isHovered ? GoldLightText : WhiteText;
    }
    else
    {
        DrawSecondaryButton(drawX, drawY, drawWidth, drawHeight, isHovered);
        textColor = isHovered ? GoldLightText : MutedText;
    }
    EndRenderColor();

    const std::wstring& text = button.GetText();
    if (!text.empty())
    {
        g_pRenderText->SetBgColor(0, 0, 0, 0);
        const int textY = static_cast<int>(drawY + (drawHeight - 10) / 2);
        RenderTextAt(static_cast<int>(drawX), textY, text.c_str(), static_cast<int>(drawWidth), textColor, isPrimary,
                     RT3_SORT_CENTER);
    }
}

void DrawCheckMark(float x, float y, float size, bool isChecked)
{
    const float radius = size / 2;
    if (isChecked)
    {
        Shapes::DrawOpenQuestMarker(x + radius, y + radius, radius);
    }
    else
    {
        Shapes::DrawFoldedQuestMarker(x + radius, y + radius, radius - 1);
    }
    EndRenderColor();
}
} // namespace GameShop::CyberStyle
