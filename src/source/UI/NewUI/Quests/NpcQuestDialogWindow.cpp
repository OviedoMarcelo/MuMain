// NpcQuestDialogWindow.cpp: implementation of the CNpcQuestDialogWindow class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "I18N/All.h"

#include "UI/NewUI/Quests/NpcQuestDialogWindow.h"

#include "Audio/DSPlaySound.h"
#include "Core/Text/TextLineWrap.h"
#include "GameLogic/Quests/NpcQuestDialogState.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "Render/Textures/ZzzTexture.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/Quests/NpcPortraitEffects.h"
#include "UI/NewUI/Quests/QuestTrackerShapes.h"
#include "UI/NewUI/UILayoutPolicy.h"
#include "UI/Scaling/UITransform.h"

#include <algorithm>
#include <cmath>

using namespace SEASON3B;
using GameLogic::Quests::NpcQuestDialog;
using GameLogic::Quests::NpcQuestDialogs;
namespace Shapes = UI::Quests::TrackerShapes;
using UI::Quests::NpcPortraitEffects::Effect;

namespace
{
constexpr int NoPortrait = -1;
constexpr const wchar_t* PortraitFileFormat = L"Interface/NpcPortraits/%u.jpg";
constexpr size_t PortraitFileNameLength = 64;
constexpr const wchar_t* EmblemFile = L"Interface/MuCyberEmblem.tga";
constexpr float EmblemSize = 48.0f;
constexpr const wchar_t* Ellipsis = L"...";

// Like the quest tracker: a dark panel with a gold border.
constexpr unsigned int FrameFill = 0xF0121110u;
constexpr unsigned int FrameBorder = 0xFFA07C44u;
constexpr float FrameCorner = 6.0f;
constexpr unsigned int PortraitFill = 0xFF0C0B0Au;
constexpr unsigned int PortraitBorder = 0xFF8C6C3Cu;
constexpr float PortraitCorner = 3.0f;
constexpr unsigned int NamePlateFill = 0xFF1A1714u;
constexpr unsigned int SeparatorColor = 0xFF5A4A30u;
constexpr float SeparatorDiamondRadius = 3.5f;
constexpr unsigned int DiamondFill = 0xFF1A140Cu;
constexpr unsigned int ButtonFill = 0xF0171513u;
constexpr unsigned int ButtonHoverFill = 0xF0282219u;
constexpr unsigned int AcceptBorder = 0xFFC9A04Eu;
constexpr unsigned int DeclineBorder = 0xFF4A4236u;
constexpr unsigned int ButtonHoverBorder = 0xFFE0B860u;
constexpr float ButtonCorner = 3.0f;
constexpr unsigned int AcceptIconColor = 0xFFF0C25Au;
constexpr unsigned int DeclineIconColor = 0xFFB8A888u;
constexpr unsigned int CloseColor = 0xFFC8A060u;
constexpr unsigned int CloseHoverColor = 0xFFFFE6A0u;
constexpr float IconLineThickness = 1.5f;
constexpr float ChevronSize = 3.5f;
constexpr float ChevronSpacing = 4.0f;
constexpr float ReturnArrowWidth = 8.0f;
constexpr float ReturnArrowHeight = 5.0f;
constexpr float CloseCrossInset = 3.0f;
// The text of a line starts this much below its top, so that it's centered.
constexpr int ButtonTextOffsetY = 5;
constexpr int NamePlateTextOffsetY = 4;

// The dialog fades in while it rises a bit into its place.
constexpr float OpeningSeconds = 0.22f;
constexpr float OpeningRise = 12.0f;
// The border of "accept" and the diamond of the separator glow from time to time.
constexpr float AcceptPulseSeconds = 2.4f;
constexpr float DiamondPulseSeconds = 3.2f;
constexpr unsigned int DiamondGlowFill = 0xFFD9A441u;
constexpr float Pi = 3.14159265f;
constexpr int AlphaShift = 24;
constexpr unsigned int ChannelMask = 0xFFu;
constexpr int ChannelBits = 8;
constexpr int ChannelCount = 4;
constexpr float FullAlpha = 255.0f;
// RenderBitmap draws opaque with an alpha of 0, so a faded bitmap needs a bit more than that.
constexpr float LowestBitmapAlpha = 0.01f;

struct TextColor
{
    BYTE Red;
    BYTE Green;
    BYTE Blue;
};

constexpr TextColor NameColor = {240, 210, 150};
constexpr TextColor QuestNameColor = {240, 192, 96};
constexpr TextColor TextWhite = {232, 226, 214};
constexpr TextColor ButtonTextColor = {232, 226, 214};

// The color with its alpha multiplied by the opacity.
unsigned int Fade(unsigned int color, float opacity)
{
    const auto alpha = static_cast<unsigned int>(((color >> AlphaShift) & ChannelMask) * opacity);
    return (color & ~(ChannelMask << AlphaShift)) | (alpha << AlphaShift);
}

// Mixes two colors: 0 is the first one, 1 the second one.
unsigned int Blend(unsigned int from, unsigned int to, float ratio)
{
    unsigned int result = 0;
    for (int channel = 0; channel < ChannelCount; ++channel)
    {
        const int shift = channel * ChannelBits;
        const float start = static_cast<float>((from >> shift) & ChannelMask);
        const float end = static_cast<float>((to >> shift) & ChannelMask);
        result |= static_cast<unsigned int>(start + (end - start) * ratio) << shift;
    }

    return result;
}

// Goes from 0 to 1 and back once per period, smoothly.
float Pulse(float seconds, float period)
{
    return 0.5f - 0.5f * std::cos(seconds / period * 2 * Pi);
}

// Fast at the start and slow at the end.
float EaseOut(float ratio)
{
    const float remaining = 1.0f - ratio;
    return 1.0f - remaining * remaining * remaining;
}

int MeasureInReferenceUnits(const wchar_t* text, size_t length)
{
    return g_pRenderText->MeasureText(text, static_cast<int>(length)).cx;
}

void RenderTextAt(int x, int y, const std::wstring& text, int width, TextColor color, bool isBold, float opacity,
                  int sort = RT3_SORT_LEFT)
{
    g_pRenderText->SetFont(isBold ? g_hFontBold : g_hFont);
    g_pRenderText->SetTextColor(color.Red, color.Green, color.Blue, static_cast<BYTE>(opacity * FullAlpha));
    g_pRenderText->RenderText(x, y, text.c_str(), width, 0, sort);
}

void DrawChevronRight(float centerX, float centerY, unsigned int color)
{
    RenderColorLineARGB(centerX - ChevronSize / 2, centerY - ChevronSize, centerX + ChevronSize / 2, centerY,
                        IconLineThickness, color);
    RenderColorLineARGB(centerX + ChevronSize / 2, centerY, centerX - ChevronSize / 2, centerY + ChevronSize,
                        IconLineThickness, color);
}

// "Go ahead": two arrow heads which point to the right.
void DrawAcceptIcon(float centerX, float centerY, unsigned int color)
{
    DrawChevronRight(centerX - ChevronSpacing / 2, centerY, color);
    DrawChevronRight(centerX + ChevronSpacing / 2, centerY, color);
}

// "Go back": an arrow which turns to the left.
void DrawDeclineIcon(float centerX, float centerY, unsigned int color)
{
    const float left = centerX - ReturnArrowWidth / 2;
    const float right = centerX + ReturnArrowWidth / 2;
    const float top = centerY - ReturnArrowHeight / 2;
    const float bottom = centerY + ReturnArrowHeight / 2;
    RenderColorLineARGB(left, top, right, top, IconLineThickness, color);
    RenderColorLineARGB(right, top, right, bottom, IconLineThickness, color);
    RenderColorLineARGB(left, top, left + ChevronSize, top - ChevronSize, IconLineThickness, color);
    RenderColorLineARGB(left, top, left + ChevronSize, top + ChevronSize, IconLineThickness, color);
}

// Cuts the lines which don't fit, and marks that the last one was cut.
void LimitLines(std::vector<std::wstring>& lines, size_t maximum)
{
    if (lines.size() <= maximum)
    {
        return;
    }

    lines.resize(maximum);
    lines.back() += Ellipsis;
}
} // namespace

CNpcQuestDialogWindow::CNpcQuestDialogWindow()
    : m_portraitNpcNumber(NoPortrait)
{
}

CNpcQuestDialogWindow::~CNpcQuestDialogWindow()
{
    Release();
}

bool CNpcQuestDialogWindow::Create(CNewUIManager* pNewUIMng)
{
    if (pNewUIMng == nullptr)
    {
        return false;
    }

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_NPC_QUEST_DIALOG, this);
    LoadBitmap(EmblemFile, BITMAP_INTERFACE_MUCYBER_EMBLEM, GL_LINEAR);
    Show(true);
    return true;
}

void CNpcQuestDialogWindow::Release()
{
    if (m_pNewUIMng == nullptr)
    {
        return;
    }

    m_pNewUIMng->RemoveUIObj(this);
    m_pNewUIMng = nullptr;
    DeleteBitmap(BITMAP_INTERFACE_MUCYBER_EMBLEM);
    if (m_portraitNpcNumber != NoPortrait)
    {
        DeleteBitmap(BITMAP_INTERFACE_NPC_PORTRAIT);
        m_portraitNpcNumber = NoPortrait;
    }
}

int CNpcQuestDialogWindow::GetLeft() const
{
    return (REFERENCE_WIDTH - PANEL_WIDTH) / 2;
}

int CNpcQuestDialogWindow::GetTop() const
{
    return UI::Scaling::DockLogicalBottom - BOTTOM_MARGIN - m_height + m_openingOffset;
}

int CNpcQuestDialogWindow::GetButtonTop(eBUTTON button) const
{
    const int index = button == eBUTTON::Accept ? 0 : 1;
    return GetTop() + m_buttonsTop + index * (BUTTON_HEIGHT + BUTTON_GAP);
}

bool CNpcQuestDialogWindow::IsMouseOverButton(eBUTTON button) const
{
    return CheckMouseIn(GetLeft() + TEXT_LEFT, GetButtonTop(button), BUTTON_WIDTH, BUTTON_HEIGHT);
}

bool CNpcQuestDialogWindow::IsMouseOverClose() const
{
    const int x = GetLeft() + PANEL_WIDTH - PADDING - CLOSE_BUTTON_SIZE;
    return CheckMouseIn(x, GetTop() + PADDING, CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE);
}

void CNpcQuestDialogWindow::Answer(bool accepted)
{
    NpcQuestDialogs().Answer(accepted);
    PlayBuffer(SOUND_CLICK01);
}

bool CNpcQuestDialogWindow::UpdateMouseEvent()
{
    if (!m_hasDialog || !CheckMouseIn(GetLeft(), GetTop(), PANEL_WIDTH, m_height))
    {
        return true;
    }

    // A click on the dialog doesn't move the character.
    if (!IsRelease(VK_LBUTTON))
    {
        return false;
    }

    if (IsMouseOverButton(eBUTTON::Accept))
    {
        Answer(true);
    }
    else if (IsMouseOverButton(eBUTTON::Decline) || IsMouseOverClose())
    {
        Answer(false);
    }

    return false;
}

bool CNpcQuestDialogWindow::UpdateKeyEvent()
{
    if (!m_hasDialog || !IsPress(VK_ESCAPE))
    {
        return true;
    }

    Answer(false);
    return false;
}

bool CNpcQuestDialogWindow::Update()
{
    auto& dialogs = NpcQuestDialogs();
    if (dialogs.GetRevision() != m_layoutRevision)
    {
        m_layoutRevision = dialogs.GetRevision();
        m_isLaidOut = false;
        m_hasDialog = dialogs.GetOpened() != nullptr;
    }

    // Walking away from the NPC or leaving its map ends the conversation.
    if (m_hasDialog && !dialogs.IsNpcInReach())
    {
        dialogs.Answer(false);
    }

    return true;
}

void CNpcQuestDialogWindow::Layout(const NpcQuestDialog& dialog)
{
    m_npcName = dialog.NpcName;
    m_questName = dialog.QuestName;

    g_pRenderText->SetFont(g_hFont);
    m_lines = WrapTextToWidth(dialog.Text, TEXT_WIDTH, MeasureInReferenceUnits);
    LimitLines(m_lines, MAXIMUM_LINES);

    const int textBottom = TEXT_TOP + static_cast<int>(m_lines.size()) * LINE_HEIGHT;
    m_separatorTop = textBottom + SEPARATOR_GAP;
    m_buttonsTop = m_separatorTop + SEPARATOR_GAP;
    const int buttonsBottom = m_buttonsTop + 2 * BUTTON_HEIGHT + BUTTON_GAP;
    m_height = (std::max)(static_cast<int>(MINIMUM_HEIGHT), buttonsBottom + PADDING);

    LoadPortrait(dialog.NpcNumber);
    m_portraitEffect = UI::Quests::NpcPortraitEffects::ForNpc(dialog.NpcNumber);
    m_openedAt = std::chrono::steady_clock::now();
}

void CNpcQuestDialogWindow::UpdateAnimation()
{
    const std::chrono::duration<float> sinceOpened = std::chrono::steady_clock::now() - m_openedAt;
    m_secondsOpen = sinceOpened.count();
    const float opening = EaseOut((std::min)(m_secondsOpen / OpeningSeconds, 1.0f));
    m_opacity = opening;
    m_openingOffset = static_cast<int>(std::lround(OpeningRise * (1.0f - opening)));
}

void CNpcQuestDialogWindow::LoadPortrait(uint16_t npcNumber)
{
    if (m_portraitNpcNumber == npcNumber)
    {
        return;
    }

    if (m_portraitNpcNumber != NoPortrait)
    {
        DeleteBitmap(BITMAP_INTERFACE_NPC_PORTRAIT);
        m_portraitNpcNumber = NoPortrait;
    }

    m_hasPortrait = false;
    if (m_npcsWithoutPortrait.count(npcNumber) > 0)
    {
        return;
    }

    wchar_t fileName[PortraitFileNameLength] = {};
    mu_swprintf(fileName, PortraitFileFormat, static_cast<unsigned int>(npcNumber));

    // Without the check, a missing file would show an error message.
    constexpr bool CheckFile = false;
    if (!LoadBitmap(fileName, BITMAP_INTERFACE_NPC_PORTRAIT, GL_LINEAR, GL_CLAMP_TO_EDGE, CheckFile))
    {
        m_npcsWithoutPortrait.insert(npcNumber);
        return;
    }

    m_hasPortrait = true;
    m_portraitNpcNumber = npcNumber;
}

void CNpcQuestDialogWindow::RenderFrame(int left, int top)
{
    Shapes::DrawPanel(static_cast<float>(left), static_cast<float>(top), PANEL_WIDTH, static_cast<float>(m_height),
                      FrameCorner, Fade(FrameFill, m_opacity), Fade(FrameBorder, m_opacity));
}

void CNpcQuestDialogWindow::RenderPortrait(int left, int top)
{
    const float x = static_cast<float>(left + PADDING);
    const float y = static_cast<float>(top + PADDING);
    Shapes::DrawPanel(x - 1, y - 1, PORTRAIT_SIZE + 2, PORTRAIT_SIZE + 2, PortraitCorner,
                      Fade(PortraitFill, m_opacity), Fade(PortraitBorder, m_opacity));

    // A bitmap is opaque with an alpha of 0, see RenderBitmap.
    const float bitmapAlpha = m_opacity >= 1.0f ? 0.0f : (std::max)(m_opacity, LowestBitmapAlpha);
    if (m_hasPortrait)
    {
        RenderBitmap(BITMAP_INTERFACE_NPC_PORTRAIT, x, y, PORTRAIT_SIZE, PORTRAIT_SIZE, 0.f, 0.f, 1.f, 1.f, true, true,
                     bitmapAlpha);
    }
    else
    {
        const float offset = (PORTRAIT_SIZE - EmblemSize) / 2;
        RenderBitmap(BITMAP_INTERFACE_MUCYBER_EMBLEM, x + offset, y + offset, EmblemSize, EmblemSize, 0.f, 0.f, 1.f,
                     1.f, true, true, bitmapAlpha);
    }

    RenderPortraitEffect(x, y);

    const float plateTop = y + PORTRAIT_SIZE + NAME_PLATE_GAP;
    Shapes::DrawPanel(x - 1, plateTop, PORTRAIT_SIZE + 2, NAME_PLATE_HEIGHT, PortraitCorner,
                      Fade(NamePlateFill, m_opacity), Fade(PortraitBorder, m_opacity));
}

void CNpcQuestDialogWindow::RenderPortraitEffect(float x, float y)
{
    if (m_portraitEffect == Effect::GoldenMotes)
    {
        m_goldenMotes.Render(x, y, PORTRAIT_SIZE, m_secondsOpen, m_opacity);
    }
}

void CNpcQuestDialogWindow::RenderTexts(int left, int top)
{
    const int plateTextTop = top + PADDING + PORTRAIT_SIZE + NAME_PLATE_GAP + NamePlateTextOffsetY;
    RenderTextAt(left + PADDING, plateTextTop, m_npcName, PORTRAIT_SIZE, NameColor, true, m_opacity,
                 RT3_SORT_CENTER);

    const int textLeft = left + TEXT_LEFT;
    RenderTextAt(textLeft, top + QUEST_NAME_TOP, m_questName, TEXT_WIDTH, QuestNameColor, true, m_opacity);
    for (size_t line = 0; line < m_lines.size(); ++line)
    {
        const int y = top + TEXT_TOP + static_cast<int>(line) * LINE_HEIGHT;
        RenderTextAt(textLeft, y, m_lines[line], TEXT_WIDTH, TextWhite, false, m_opacity);
    }
}

void CNpcQuestDialogWindow::RenderSeparator(int left, int top)
{
    const float y = static_cast<float>(top + m_separatorTop);
    const float x = static_cast<float>(left + TEXT_LEFT);
    const float centerX = x + BUTTON_WIDTH / 2.0f;
    const unsigned int line = Fade(SeparatorColor, m_opacity);
    RenderColorLineARGB(x, y, centerX - 2 * SeparatorDiamondRadius, y, 1.0f, line);
    RenderColorLineARGB(centerX + 2 * SeparatorDiamondRadius, y, x + BUTTON_WIDTH, y, 1.0f, line);

    const unsigned int fill = Blend(DiamondFill, DiamondGlowFill, Pulse(m_secondsOpen, DiamondPulseSeconds));
    Shapes::DrawDiamond(centerX, y, SeparatorDiamondRadius, Fade(FrameBorder, m_opacity), Fade(fill, m_opacity),
                        Fade(DiamondFill, m_opacity));
}

void CNpcQuestDialogWindow::RenderButton(int left, eBUTTON button)
{
    const bool isAccept = button == eBUTTON::Accept;
    const bool isHovered = IsMouseOverButton(button);
    const int x = left + TEXT_LEFT;
    const int y = GetButtonTop(button);
    Shapes::DrawPanel(static_cast<float>(x), static_cast<float>(y), BUTTON_WIDTH, BUTTON_HEIGHT, ButtonCorner,
                      Fade(isHovered ? ButtonHoverFill : ButtonFill, m_opacity),
                      Fade(GetButtonBorder(button, isHovered), m_opacity));

    const float iconX = x + BUTTON_ICON_WIDTH / 2.0f;
    const float iconY = y + BUTTON_HEIGHT / 2.0f;
    if (isAccept)
    {
        DrawAcceptIcon(iconX, iconY, Fade(AcceptIconColor, m_opacity));
    }
    else
    {
        DrawDeclineIcon(iconX, iconY, Fade(DeclineIconColor, m_opacity));
    }

    const std::wstring text = isAccept ? I18N::Game::NpcQuestDialogAccept : I18N::Game::NpcQuestDialogDecline;
    RenderTextAt(x + BUTTON_ICON_WIDTH, y + ButtonTextOffsetY, text, BUTTON_WIDTH - BUTTON_ICON_WIDTH,
                 ButtonTextColor, false, m_opacity);
}

unsigned int CNpcQuestDialogWindow::GetButtonBorder(eBUTTON button, bool isHovered) const
{
    if (isHovered)
    {
        return ButtonHoverBorder;
    }

    if (button == eBUTTON::Decline)
    {
        return DeclineBorder;
    }

    // "Accept" is the answer the dialog waits for, so its border glows slowly.
    return Blend(AcceptBorder, ButtonHoverBorder, Pulse(m_secondsOpen, AcceptPulseSeconds));
}

void CNpcQuestDialogWindow::RenderCloseButton(int left, int top)
{
    const float x = static_cast<float>(left + PANEL_WIDTH - PADDING - CLOSE_BUTTON_SIZE);
    const float y = static_cast<float>(top + PADDING);
    const unsigned int color = Fade(IsMouseOverClose() ? CloseHoverColor : CloseColor, m_opacity);
    const float crossStart = CloseCrossInset;
    const float crossEnd = CLOSE_BUTTON_SIZE - CloseCrossInset;
    RenderColorLineARGB(x + crossStart, y + crossStart, x + crossEnd, y + crossEnd, IconLineThickness, color);
    RenderColorLineARGB(x + crossEnd, y + crossStart, x + crossStart, y + crossEnd, IconLineThickness, color);
}

bool CNpcQuestDialogWindow::Render()
{
    const auto* dialog = NpcQuestDialogs().GetOpened();
    if (dialog == nullptr)
    {
        return true;
    }

    if (!m_isLaidOut)
    {
        Layout(*dialog);
        m_isLaidOut = true;
    }

    UpdateAnimation();

    EnableAlphaTest();
    g_pRenderText->SetBgColor(0);
    const int left = GetLeft();
    const int top = GetTop();
    RenderFrame(left, top);
    RenderPortrait(left, top);
    RenderSeparator(left, top);
    RenderButton(left, eBUTTON::Accept);
    RenderButton(left, eBUTTON::Decline);
    RenderCloseButton(left, top);
    RenderTexts(left, top);

    EndRenderColor();
    g_pRenderText->SetFont(g_hFont);
    DisableAlphaBlend();
    return true;
}

float CNpcQuestDialogWindow::GetLayerDepth()
{
    return UI::Layout::ForegroundPanelLayerDepth;
}

float CNpcQuestDialogWindow::GetKeyEventOrder()
{
    // Before the other windows, so that escape closes the dialog first.
    return UI::Layout::ForegroundPanelLayerDepth;
}
