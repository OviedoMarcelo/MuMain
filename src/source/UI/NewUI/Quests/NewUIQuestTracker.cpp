// NewUIQuestTracker.cpp: implementation of the CNewUIQuestTracker class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "I18N/All.h"

#include "UI/NewUI/Quests/NewUIQuestTracker.h"

#include "Audio/DSPlaySound.h"
#include "Engine/Object/ZzzInventory.h"
#include "GameLogic/Quests/WeeklyQuestCatalog.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/NewUI/Quests/QuestTrackerShapes.h"
#include "UI/Quests/QuestTypeName.h"

#include <algorithm>
#include <cwctype>

using namespace SEASON3B;
using GameLogic::Quests::WeeklyQuests;
using UI::Quests::TrackedQuest;
namespace Shapes = UI::Quests::TrackerShapes;

namespace
{
// The party list takes this much at the right of the free screen, see CNewUIPartyListWindow::SetPos.
constexpr int PartyListWidth = 77 + 2;
constexpr const wchar_t* Ellipsis = L"...";
constexpr const wchar_t* RewardPendingMarker = L"!";

// The frame around the tracker and the panels in it.
constexpr unsigned int FrameFill = 0xE6121110u;
constexpr unsigned int FrameBorder = 0xFFA07C44u;
constexpr float FrameCorner = 5.0f;
constexpr unsigned int HeaderLine = 0xFF7A5E34u;
constexpr float HeaderLineInset = 3.0f;
constexpr float HeaderOrnamentRadius = 2.0f;
constexpr unsigned int CardFill = 0xF00C0B0Au;
constexpr unsigned int CardHoverFill = 0xF0181614u;
constexpr unsigned int CardBorder = 0xFF5A4E3Cu;
constexpr float CardCorner = 3.0f;
constexpr unsigned int CardSeparator = 0xFF3C3832u;
constexpr unsigned int RowFill = 0xE6141312u;
constexpr unsigned int RowHoverFill = 0xF0242120u;
constexpr unsigned int RowBorder = 0xFF4A4236u;
constexpr unsigned int BadgeFill = 0xFF1C1D20u;
constexpr unsigned int BadgeBorder = 0xFF5A5C64u;
constexpr float BadgeCorner = 2.0f;
constexpr unsigned int FoldButtonFill = 0xFF1A1816u;
constexpr unsigned int FoldButtonBorder = 0xFF6A5A40u;
constexpr unsigned int ArrowColor = 0xFFC8B89Au;
constexpr float FoldArrowSize = 4.0f;
constexpr float RowArrowSize = 3.5f;

// The icons: the MU Cyber emblem in the title, the markers of the quests.
constexpr const wchar_t* EmblemFile = L"Interface/MuCyberEmblem.tga";
constexpr float EmblemX = 3.0f;
constexpr float EmblemSize = 20.0f;
constexpr int TitleX = 26;
constexpr int TitleY = 6;
constexpr float OpenMarkerX = 10.0f;
constexpr float OpenMarkerRadius = 6.0f;
constexpr float FoldedMarkerX = 10.0f;
constexpr float FoldedMarkerRadius = 4.5f;
// The text of a line starts this much below its top, so that it's centered on the icons.
constexpr int TextOffsetY = 1;
constexpr int RowTextY = 4;
constexpr int LabelHeight = 12;
constexpr int FooterTextX = 18;

struct TextColor
{
    BYTE Red;
    BYTE Green;
    BYTE Blue;
};

constexpr TextColor TitleColor = {240, 192, 96};
constexpr TextColor NameColor = {240, 192, 96};
constexpr TextColor TextWhite = {232, 226, 214};
constexpr TextColor LabelColor = {138, 146, 178};
constexpr TextColor BadgeColor = {196, 200, 214};
constexpr TextColor FooterColor = {150, 160, 196};
constexpr TextColor FoldedNameColor = {220, 214, 200};
constexpr TextColor PendingColor = {255, 190, 100};

int Measure(const std::wstring& text)
{
    return g_pRenderText->MeasureText(text.c_str(), static_cast<int>(text.size())).cx;
}

// Cuts the text, so that it fits into the width, and marks that it was cut.
std::wstring Fit(const std::wstring& text, int width)
{
    if (Measure(text) <= width)
    {
        return text;
    }

    std::wstring cut = text;
    while (!cut.empty() && Measure(cut + Ellipsis) > width)
    {
        cut.pop_back();
    }

    return cut + Ellipsis;
}

std::wstring ToUpper(std::wstring text)
{
    std::transform(text.begin(), text.end(), text.begin(),
                   [](wchar_t character) { return static_cast<wchar_t>(std::towupper(character)); });
    return text;
}

std::wstring FormatCount(uint32_t current, uint32_t required)
{
    return std::to_wstring(current) + L"/" + std::to_wstring(required);
}

void RenderTextAt(int x, int y, const std::wstring& text, int width, TextColor color, bool isBold,
                  int sort = RT3_SORT_LEFT)
{
    g_pRenderText->SetFont(isBold ? g_hFontBold : g_hFont);
    g_pRenderText->SetTextColor(color.Red, color.Green, color.Blue, 255);
    g_pRenderText->RenderText(x, y, text.c_str(), width, 0, sort);
}
} // namespace

CNewUIQuestTracker::CNewUIQuestTracker() = default;

CNewUIQuestTracker::~CNewUIQuestTracker()
{
    Release();
}

bool CNewUIQuestTracker::Create(CNewUIManager* pNewUIMng)
{
    if (pNewUIMng == nullptr)
    {
        return false;
    }

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_QUEST_TRACKER, this);
    LoadBitmap(EmblemFile, BITMAP_INTERFACE_MUCYBER_EMBLEM, GL_LINEAR);
    Show(true);
    return true;
}

void CNewUIQuestTracker::Release()
{
    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = nullptr;
        DeleteBitmap(BITMAP_INTERFACE_MUCYBER_EMBLEM);
    }
}

int CNewUIQuestTracker::GetLeft() const
{
    // Without a party it stays at the right edge, behind the windows which open there.
    const int right = PartyNumber > 0 ? GetScreenWidth() - PartyListWidth - TRACKER_PARTY_GAP : REFERENCE_WIDTH - 2;
    return right - TRACKER_WIDTH;
}

int CNewUIQuestTracker::GetContentWidth() const
{
    return TRACKER_WIDTH - 2 * TRACKER_MARGIN;
}

bool CNewUIQuestTracker::IsMouseOverFoldButton() const
{
    const int x = GetLeft() + TRACKER_WIDTH - TRACKER_MARGIN - FOLD_BUTTON_SIZE;
    const int y = TRACKER_TOP + (HEADER_HEIGHT - FOLD_BUTTON_SIZE) / 2;
    return CheckMouseIn(x, y, FOLD_BUTTON_SIZE, FOLD_BUTTON_SIZE);
}

bool CNewUIQuestTracker::IsMouseOverQuest(int top, int height) const
{
    return CheckMouseIn(GetLeft() + TRACKER_MARGIN, TRACKER_TOP + top, GetContentWidth(), height);
}

const std::wstring* CNewUIQuestTracker::FindQuestUnderMouse() const
{
    if (m_isFolded)
    {
        return nullptr;
    }

    for (const auto& quest : m_openQuests)
    {
        if (IsMouseOverQuest(quest.Top, quest.Height))
        {
            return &quest.Id;
        }
    }

    for (const auto& quest : m_foldedQuests)
    {
        if (IsMouseOverQuest(quest.Top, ROW_HEIGHT))
        {
            return &quest.Id;
        }
    }

    return nullptr;
}

void CNewUIQuestTracker::Invalidate()
{
    m_isLaidOut = false;
}

void CNewUIQuestTracker::ToggleQuest(const std::wstring& id)
{
    if (m_openQuestIds.erase(id) == 0)
    {
        m_openQuestIds.insert(id);
    }

    Invalidate();
}

bool CNewUIQuestTracker::UpdateMouseEvent()
{
    if (!m_hasQuests || !CheckMouseIn(GetLeft(), TRACKER_TOP, TRACKER_WIDTH, m_height))
    {
        return true;
    }

    // A click on the tracker doesn't move the character.
    if (!IsRelease(VK_LBUTTON))
    {
        return false;
    }

    if (IsMouseOverFoldButton())
    {
        m_isFolded = !m_isFolded;
        Invalidate();
        PlayBuffer(SOUND_CLICK01);
    }
    else if (const auto* id = FindQuestUnderMouse())
    {
        // Copied, because toggling lays out the quests again.
        ToggleQuest(std::wstring(*id));
        PlayBuffer(SOUND_CLICK01);
    }

    return false;
}

bool CNewUIQuestTracker::UpdateKeyEvent()
{
    return true;
}

bool CNewUIQuestTracker::Update()
{
    if (m_isLaidOut && m_layoutRevision != WeeklyQuests().GetRevision())
    {
        Invalidate();
    }

    return true;
}

int CNewUIQuestTracker::LayoutOpenQuest(const TrackedQuest& quest, int top)
{
    const int width = GetContentWidth() - CARD_TEXT_INDENT - CARD_PADDING;
    auto& card = m_openQuests.emplace_back();
    card.Id = quest.Id;
    card.Top = top;

    g_pRenderText->SetFont(g_hFont);
    card.Badge = ToUpper(UI::Quests::GetTypeName(quest.Category));
    card.BadgeWidth = Measure(card.Badge) + 2 * BADGE_PADDING;
    g_pRenderText->SetFont(g_hFontBold);
    // The arrow which folds the quest is at the right of its name.
    card.Name = Fit(quest.Name, width - card.BadgeWidth - BADGE_GAP - ROW_ARROW_WIDTH);

    g_pRenderText->SetFont(g_hFont);
    card.IsRewardPending = quest.IsRewardPending;
    card.Objective = Fit(quest.IsRewardPending ? std::wstring(I18N::Game::QuestTrackerRewardPending)
                                                      : quest.Objective,
                                width);
    card.HasProgress = quest.ObjectiveRequired > 0;
    if (card.HasProgress)
    {
        card.Count = FormatCount(quest.ObjectiveCurrent, quest.ObjectiveRequired);
        card.Ratio = static_cast<float>(quest.ObjectiveCurrent) / quest.ObjectiveRequired;
    }

    card.FooterTop = card.HasProgress ? CARD_BAR_Y + CARD_BAR_HEIGHT + CARD_SECTION_GAP : CARD_PROGRESS_Y;
    card.Footer = Fit(quest.Rewards, GetContentWidth() - FooterTextX - 2 * CARD_PADDING);
    card.Height = card.FooterTop + (card.Footer.empty() ? 0 : CARD_FOOTER_HEIGHT);
    return card.Height;
}

void CNewUIQuestTracker::LayoutFoldedQuest(const TrackedQuest& quest, int top)
{
    const int nameWidth = GetContentWidth() - CARD_TEXT_INDENT - ROW_COUNT_WIDTH - ROW_ARROW_WIDTH;
    g_pRenderText->SetFont(g_hFont);

    FoldedQuest folded;
    folded.Id = quest.Id;
    folded.Name = Fit(quest.Name, nameWidth);
    folded.IsRewardPending = quest.IsRewardPending;
    if (quest.IsRewardPending)
    {
        folded.Count = RewardPendingMarker;
    }
    else if (quest.QuestRequired > 0)
    {
        folded.Count = FormatCount(quest.QuestCurrent, quest.QuestRequired);
    }

    folded.Top = top;
    m_foldedQuests.push_back(std::move(folded));
}

void CNewUIQuestTracker::Layout()
{
    m_hasQuests = false;
    m_openQuests.clear();
    m_foldedQuests.clear();
    m_layoutRevision = WeeklyQuests().GetRevision();
    m_isLaidOut = true;
    m_height = HEADER_HEIGHT;
    if (!WeeklyQuests().IsAvailable())
    {
        return;
    }

    const auto content = UI::Quests::BuildQuestTracker(WeeklyQuests().GetQuests(), TRACKER_MAX_QUESTS);
    if (content.IsEmpty())
    {
        return;
    }

    m_hasQuests = true;
    g_pRenderText->SetFont(g_hFontBold);
    m_title = Fit(I18N::Game::QuestTrackerTitle, TRACKER_WIDTH - TitleX - FOLD_BUTTON_SIZE - 2 * TRACKER_MARGIN);
    if (m_isFolded)
    {
        return;
    }

    if (!m_hasChosenOpenQuests)
    {
        m_openQuestIds.insert(content.Quests.front().Id);
        m_hasChosenOpenQuests = true;
    }

    // Every quest keeps its place, open or folded.
    int top = HEADER_HEIGHT + TRACKER_MARGIN - ROW_GAP;
    for (const auto& quest : content.Quests)
    {
        top += ROW_GAP;
        if (m_openQuestIds.contains(quest.Id))
        {
            top += LayoutOpenQuest(quest, top);
        }
        else
        {
            LayoutFoldedQuest(quest, top);
            top += ROW_HEIGHT;
        }
    }

    m_height = top + TRACKER_MARGIN;
}

void CNewUIQuestTracker::RenderFrame(int left)
{
    Shapes::DrawPanel(static_cast<float>(left), static_cast<float>(TRACKER_TOP), static_cast<float>(TRACKER_WIDTH),
                      static_cast<float>(m_height), FrameCorner, FrameFill, FrameBorder);
    if (m_isFolded)
    {
        return;
    }

    const float lineY = static_cast<float>(TRACKER_TOP + HEADER_HEIGHT);
    const float lineLeft = left + HeaderLineInset;
    const float lineRight = left + TRACKER_WIDTH - HeaderLineInset;
    RenderColorLineARGB(lineLeft, lineY, lineRight, lineY, 1.0f, HeaderLine);
    Shapes::DrawDiamond(lineLeft, lineY, HeaderOrnamentRadius, FrameBorder, FrameFill, FrameFill);
    Shapes::DrawDiamond(lineRight, lineY, HeaderOrnamentRadius, FrameBorder, FrameFill, FrameFill);
}

void CNewUIQuestTracker::RenderHeader(int left)
{
    const float centerY = static_cast<float>(TRACKER_TOP) + static_cast<float>(HEADER_HEIGHT) / 2;
    // The shapes of the frame turn the textures off.
    EndRenderColor();
    RenderBitmap(BITMAP_INTERFACE_MUCYBER_EMBLEM,left + EmblemX, centerY - EmblemSize / 2, EmblemSize, EmblemSize);

    const float buttonX = static_cast<float>(left + TRACKER_WIDTH - TRACKER_MARGIN - FOLD_BUTTON_SIZE);
    const float buttonSize = static_cast<float>(FOLD_BUTTON_SIZE);
    const float buttonY = centerY - buttonSize / 2;
    Shapes::DrawPanel(buttonX, buttonY, buttonSize, buttonSize, CardCorner, FoldButtonFill,
                      FoldButtonBorder);
    Shapes::DrawChevron(buttonX + buttonSize / 2, centerY, FoldArrowSize, !m_isFolded, ArrowColor);

    RenderTextAt(left + TitleX, TRACKER_TOP + TitleY, m_title, TRACKER_WIDTH - TitleX, TitleColor, true);
}

void CNewUIQuestTracker::RenderOpenQuest(int left, const OpenQuest& card)
{
    const int x = left + TRACKER_MARGIN;
    const int top = TRACKER_TOP + card.Top;
    const int width = GetContentWidth();
    const bool isHovered = IsMouseOverQuest(card.Top, card.Height);
    Shapes::DrawPanel(static_cast<float>(x), static_cast<float>(top), static_cast<float>(width),
                      static_cast<float>(card.Height), CardCorner, isHovered ? CardHoverFill : CardFill,
                      CardBorder);
    const float nameCenterY = top + CARD_NAME_Y + LabelHeight / 2.0f;
    Shapes::DrawOpenQuestMarker(x + OpenMarkerX, nameCenterY, OpenMarkerRadius);
    Shapes::DrawChevron(static_cast<float>(x + width - ROW_ARROW_WIDTH / 2), nameCenterY, RowArrowSize, true,
                        ArrowColor);

    const int textX = x + CARD_TEXT_INDENT;
    RenderTextAt(textX, top + CARD_NAME_Y + TextOffsetY, card.Name, width, NameColor, true);

    g_pRenderText->SetFont(g_hFontBold);
    const int badgeX = textX + Measure(card.Name) + BADGE_GAP;
    const int badgeY = top + CARD_NAME_Y;
    Shapes::DrawPanel(static_cast<float>(badgeX), static_cast<float>(badgeY), static_cast<float>(card.BadgeWidth),
                      BADGE_HEIGHT, BadgeCorner, BadgeFill, BadgeBorder);
    RenderTextAt(badgeX, badgeY + TextOffsetY, card.Badge, card.BadgeWidth, BadgeColor, false,
                 RT3_SORT_CENTER);

    const auto objectiveColor = card.IsRewardPending ? PendingColor : TextWhite;
    RenderTextAt(textX, top + CARD_OBJECTIVE_Y, card.Objective, width - CARD_TEXT_INDENT, objectiveColor, false);

    const int textWidth = width - CARD_TEXT_INDENT - CARD_PADDING;
    if (card.HasProgress)
    {
        RenderOpenQuestProgress(textX, top, textWidth, card);
    }

    if (!card.Footer.empty())
    {
        RenderOpenQuestFooter(x, top, width, card);
    }
}

void CNewUIQuestTracker::RenderOpenQuestProgress(int x, int top, int width, const OpenQuest& card)
{
    RenderTextAt(x, top + CARD_PROGRESS_Y, I18N::Game::QuestTrackerProgress, width, LabelColor, false);
    RenderTextAt(x, top + CARD_PROGRESS_Y, card.Count, width, TextWhite, false, RT3_SORT_RIGHT);
    Shapes::DrawProgressBar(static_cast<float>(x), static_cast<float>(top + CARD_BAR_Y), static_cast<float>(width),
                            CARD_BAR_HEIGHT, card.Ratio);
}

void CNewUIQuestTracker::RenderOpenQuestFooter(int x, int top, int width, const OpenQuest& card)
{
    const float lineY = static_cast<float>(top + card.FooterTop);
    RenderColorLineARGB(static_cast<float>(x + CARD_PADDING), lineY, static_cast<float>(x + width - CARD_PADDING), lineY,
                        1.0f, CardSeparator);

    const int footerY = top + card.FooterTop + (CARD_FOOTER_HEIGHT - LabelHeight) / 2;
    const float chestY = footerY + (LabelHeight - Shapes::ChestHeight) / 2.0f;
    Shapes::DrawChest(static_cast<float>(x + CARD_PADDING), chestY);
    RenderTextAt(x + CARD_PADDING + FooterTextX, footerY + TextOffsetY, card.Footer,
                 width - FooterTextX - 2 * CARD_PADDING, FooterColor, false);
}

void CNewUIQuestTracker::RenderFoldedQuest(int left, const FoldedQuest& quest)
{
    const int x = left + TRACKER_MARGIN;
    const int top = TRACKER_TOP + quest.Top;
    const int width = GetContentWidth();
    const bool isHovered = CheckMouseIn(x, top, width, ROW_HEIGHT);
    Shapes::DrawPanel(static_cast<float>(x), static_cast<float>(top), static_cast<float>(width), ROW_HEIGHT, CardCorner,
                      isHovered ? RowHoverFill : RowFill, RowBorder);

    const float centerY = static_cast<float>(top) + static_cast<float>(ROW_HEIGHT) / 2;
    Shapes::DrawFoldedQuestMarker(x + FoldedMarkerX, centerY, FoldedMarkerRadius);
    Shapes::DrawChevron(static_cast<float>(x + width - ROW_ARROW_WIDTH / 2), centerY, RowArrowSize, false, ArrowColor);

    RenderTextAt(x + CARD_TEXT_INDENT, top + RowTextY, quest.Name, width, FoldedNameColor, false);
    const int countX = x + width - ROW_ARROW_WIDTH - ROW_COUNT_WIDTH;
    RenderTextAt(countX, top + RowTextY, quest.Count, ROW_COUNT_WIDTH, quest.IsRewardPending ? PendingColor : TextWhite,
                 false, RT3_SORT_RIGHT);
}

bool CNewUIQuestTracker::Render()
{
    if (!m_isLaidOut)
    {
        Layout();
    }

    if (!m_hasQuests)
    {
        return true;
    }

    EnableAlphaTest();
    g_pRenderText->SetBgColor(0);
    const int left = GetLeft();
    RenderFrame(left);
    RenderHeader(left);
    if (!m_isFolded)
    {
        for (const auto& quest : m_openQuests)
        {
            RenderOpenQuest(left, quest);
        }
        for (const auto& quest : m_foldedQuests)
        {
            RenderFoldedQuest(left, quest);
        }
    }

    EndRenderColor();
    g_pRenderText->SetFont(g_hFont);
    DisableAlphaBlend();
    return true;
}

float CNewUIQuestTracker::GetLayerDepth()
{
    // Below the windows which open at the right, like the inventory (4.2).
    return 1.0f;
}
