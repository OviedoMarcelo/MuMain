// WeeklyQuestWindow.cpp: implementation of the CWeeklyQuestWindow class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "I18N/All.h"

#include "UI/NewUI/Dialogs/WeeklyQuestWindow.h"

#include "Audio/DSPlaySound.h"
#include "Core/Text/TextLineWrap.h"
#include "UI/NewUI/NewUISystem.h"

#include <algorithm>
#include <chrono>

using namespace SEASON3B;
using GameLogic::Quests::WeeklyQuest;
using GameLogic::Quests::WeeklyQuests;

namespace
{
constexpr const wchar_t* HotKeyName = L"Y";
// What stands at the right of a quest which is done, instead of its progress.
constexpr const wchar_t* CompletedMarker = L"OK";
constexpr const wchar_t* PendingMarker = L"!";
constexpr const wchar_t* ArrowMarker = L">";
constexpr size_t FormattedTextLength = 128;

struct TextColor
{
    BYTE Red;
    BYTE Green;
    BYTE Blue;
};

constexpr TextColor TitleColor = {255, 220, 120};
constexpr TextColor NormalColor = {220, 220, 220};
constexpr TextColor CompletedColor = {150, 230, 150};
constexpr TextColor PendingColor = {255, 190, 100};
constexpr TextColor DescriptionColor = {200, 220, 255};
// The arrows stay in the background, the hint has to be read on the stone of the frame.
constexpr TextColor ArrowColor = {160, 160, 160};
constexpr TextColor HintColor = {240, 225, 180};
constexpr TextColor HoveredColor = {255, 255, 0};
// The colors of the quest window of the original client.
constexpr TextColor QuestNameColor = {36, 242, 252};
constexpr TextColor QuestTextColor = {255, 255, 255};
constexpr TextColor QuestHeadingColor = {255, 255, 0};

// The background of the quest row under the mouse, as ARGB: a translucent gold
// tint, so that the yellow text of the row stays readable on it.
constexpr unsigned int HoverBackgroundColor = 0x46C8A03Cu;
// The dark panel behind the content, the one of the boxes of the pet window.
constexpr unsigned int PanelBackdropColor = 0x99000000u;

void UseTextColor(const TextColor& color)
{
    g_pRenderText->SetTextColor(color.Red, color.Green, color.Blue, 255);
    g_pRenderText->SetBgColor(0);
}

using DetailStyle = CWeeklyQuestWindow::eDETAIL_STYLE;

bool IsBold(DetailStyle style)
{
    return style != CWeeklyQuestWindow::STYLE_DESCRIPTION && style != CWeeklyQuestWindow::STYLE_VALUE;
}

// The name and the description are centered like in the quest window of the
// original client, the headings and their values start at the left.
int GetSort(DetailStyle style)
{
    const bool isCentered = style == CWeeklyQuestWindow::STYLE_TITLE || style == CWeeklyQuestWindow::STYLE_DESCRIPTION;
    return isCentered ? RT3_SORT_CENTER : RT3_SORT_LEFT;
}

const TextColor& GetColor(DetailStyle style)
{
    switch (style)
    {
    case CWeeklyQuestWindow::STYLE_TITLE:
        return QuestNameColor;
    case CWeeklyQuestWindow::STYLE_HEADING:
        return QuestHeadingColor;
    case CWeeklyQuestWindow::STYLE_COMPLETED:
        return CompletedColor;
    case CWeeklyQuestWindow::STYLE_PENDING:
        return PendingColor;
    default:
        return QuestTextColor;
    }
}

void UseFont(DetailStyle style)
{
    g_pRenderText->SetFont(IsBold(style) ? g_hFontBold : g_hFont);
}

const TextColor& GetQuestColor(const WeeklyQuest& quest)
{
    if (quest.IsRewardPending())
    {
        return PendingColor;
    }

    return quest.IsCompleted ? CompletedColor : NormalColor;
}

// Renders one line of the window. An empty text has to be skipped: the text
// renderer measures a placeholder for it and would leave a stray glyph
// behind whenever a box width is given.
void RenderLine(int x, int y, const wchar_t* text, int boxWidth, int boxHeight = 0, int sort = RT3_SORT_LEFT)
{
    if (text == nullptr || text[0] == L'\0')
    {
        return;
    }

    g_pRenderText->RenderText(x, y, text, boxWidth, boxHeight, sort);
}

int MeasureInReferenceUnits(const wchar_t* text, size_t length)
{
    return g_pRenderText->MeasureText(text, static_cast<int>(length)).cx;
}

std::wstring GetProgressText(const WeeklyQuest& quest)
{
    if (quest.IsRewardPending())
    {
        return PendingMarker;
    }

    if (quest.IsCompleted)
    {
        return CompletedMarker;
    }

    return std::to_wstring(quest.CurrentCount) + L"/" + std::to_wstring(quest.RequiredCount);
}

std::wstring FormatProgressLine(const WeeklyQuest& quest)
{
    wchar_t text[FormattedTextLength] = {};
    mu_swprintf_s(text, I18N::Game::WeeklyQuestsProgress, quest.CurrentCount, quest.RequiredCount);
    return text;
}

std::wstring FormatTimeUntilReset()
{
    constexpr int HoursPerDay = 24;
    constexpr int MinutesPerHour = 60;
    const auto remaining = WeeklyQuests().GetTimeUntilReset();
    const auto totalHours = static_cast<int>(std::chrono::duration_cast<std::chrono::hours>(remaining).count());
    const auto totalMinutes = static_cast<int>(std::chrono::duration_cast<std::chrono::minutes>(remaining).count());

    wchar_t text[FormattedTextLength] = {};
    mu_swprintf_s(text, I18N::Game::WeeklyQuestsResetIn, totalHours / HoursPerDay, totalHours % HoursPerDay,
                  totalMinutes % MinutesPerHour);
    return text;
}
} // namespace

SEASON3B::CWeeklyQuestWindow::CWeeklyQuestWindow()
{
    m_pNewUIMng = nullptr;
    m_Pos.x = 0;
    m_Pos.y = 0;
    m_page = PAGE_LIST;
    m_selectedRow = -1;
    m_scrollOffset = 0;
    m_shownRevision = 0;
}

SEASON3B::CWeeklyQuestWindow::~CWeeklyQuestWindow()
{
    Release();
}

bool SEASON3B::CWeeklyQuestWindow::Create(CNewUIManager* pNewUIMng, int x, int y)
{
    if (pNewUIMng == nullptr)
    {
        return false;
    }

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_WEEKLY_QUESTS, this);

    LoadImages();
    SetPos(x, y);
    InitButtons();
    Show(false);

    return true;
}

void SEASON3B::CWeeklyQuestWindow::Release()
{
    UnloadImages();

    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = nullptr;
    }
}

void SEASON3B::CWeeklyQuestWindow::SetPos(int x, int y)
{
    m_Pos.x = x;
    m_Pos.y = y;

    m_BtnExit.ChangeButtonInfo(m_Pos.x + EXIT_BUTTON_X, m_Pos.y + EXIT_BUTTON_Y, EXIT_BUTTON_WIDTH, EXIT_BUTTON_HEIGHT);
    m_BtnBack.ChangeButtonInfo(m_Pos.x + CONTENT_LEFT, m_Pos.y + BUTTON_ROW_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
}

void SEASON3B::CWeeklyQuestWindow::InitButtons()
{
    wchar_t closeText[256] = {};
    mu_swprintf_s(closeText, I18N::Game::CloseS, HotKeyName);

    m_BtnExit.ChangeButtonImgState(true, IMAGE_WEEKLYQUEST_BTN_EXIT);
    m_BtnExit.ChangeToolTipText(closeText, true);

    m_BtnBack.ChangeButtonImgState(true, IMAGE_WEEKLYQUEST_BTN, true);
    m_BtnBack.ChangeText(&I18N::Game::ChatCommandsBack);
}

float SEASON3B::CWeeklyQuestWindow::GetLayerDepth()
{
    return LayerDepth;
}

float SEASON3B::CWeeklyQuestWindow::GetKeyEventOrder()
{
    return 10.f;
}

void SEASON3B::CWeeklyQuestWindow::OpenningProcess()
{
    m_selectedRow = -1;
    WrapHint();
    ShowPage(PAGE_LIST);
}

void SEASON3B::CWeeklyQuestWindow::ClosingProcess()
{
    m_detailLines.clear();
}

const WeeklyQuest* SEASON3B::CWeeklyQuestWindow::GetQuestAt(int row) const
{
    const auto& quests = WeeklyQuests().GetQuests();
    if (row < 0 || static_cast<size_t>(row) >= quests.size())
    {
        return nullptr;
    }

    return &quests[row];
}

const WeeklyQuest* SEASON3B::CWeeklyQuestWindow::GetSelectedQuest() const
{
    return GetQuestAt(m_selectedRow);
}

void SEASON3B::CWeeklyQuestWindow::ShowPage(ePAGE page)
{
    m_page = page;
    m_scrollOffset = 0;

    if (page == PAGE_DETAILS)
    {
        WrapDetailsOfSelected();
    }
}

void SEASON3B::CWeeklyQuestWindow::PickQuest(int row)
{
    m_selectedRow = row;
    if (GetSelectedQuest() != nullptr)
    {
        ShowPage(PAGE_DETAILS);
    }
}

void SEASON3B::CWeeklyQuestWindow::WrapDetailsOfSelected()
{
    m_detailLines.clear();
    m_shownRevision = WeeklyQuests().GetRevision();

    const auto* quest = GetSelectedQuest();
    if (quest == nullptr)
    {
        return;
    }

    AddDetailLines(quest->Name, STYLE_TITLE);
    m_detailLines.push_back({});
    AddDetailLines(quest->Description, STYLE_DESCRIPTION);
    m_detailLines.push_back({});

    AddDetailLines(FormatProgressLine(*quest), STYLE_HEADING);
    if (quest->IsRewardPending())
    {
        AddDetailLines(I18N::Game::WeeklyQuestsRewardPending, STYLE_PENDING);
    }
    else if (quest->IsCompleted)
    {
        AddDetailLines(I18N::Game::WeeklyQuestsCompleted, STYLE_COMPLETED);
    }

    m_detailLines.push_back({});
    AddDetailLines(I18N::Game::WeeklyQuestsRewards, STYLE_HEADING);
    AddDetailLines(quest->Rewards, STYLE_VALUE);
}

void SEASON3B::CWeeklyQuestWindow::AddDetailLines(const std::wstring& text, eDETAIL_STYLE style)
{
    // A bold line is wider, so it has to be measured with the font it's drawn with.
    UseFont(style);
    for (auto& line : WrapTextToWidth(text, CONTENT_WIDTH, MeasureInReferenceUnits))
    {
        m_detailLines.push_back({std::move(line), style});
    }

    g_pRenderText->SetFont(g_hFont);
}

void SEASON3B::CWeeklyQuestWindow::WrapHint()
{
    g_pRenderText->SetFont(g_hFont);
    m_hintLines = WrapTextToWidth(I18N::Game::WeeklyQuestsClickHint, CONTENT_WIDTH, MeasureInReferenceUnits);
    if (m_hintLines.size() > HINT_LINES)
    {
        m_hintLines.resize(HINT_LINES);
    }
}

int SEASON3B::CWeeklyQuestWindow::GetScrollableRowCount() const
{
    if (m_page == PAGE_DETAILS)
    {
        return static_cast<int>(m_detailLines.size());
    }

    return static_cast<int>(WeeklyQuests().GetQuests().size());
}

int SEASON3B::CWeeklyQuestWindow::GetVisibleRowCount() const
{
    // The details page has no hint, so it can use the space down to its button.
    return m_page == PAGE_DETAILS ? DETAIL_VISIBLE_ROWS : VISIBLE_ROWS;
}

bool SEASON3B::CWeeklyQuestWindow::IsRowHovered(int y) const
{
    return CheckMouseIn(m_Pos.x + CONTENT_LEFT, y, CONTENT_WIDTH, ROW_HEIGHT);
}

void SEASON3B::CWeeklyQuestWindow::Scroll(int rows)
{
    const auto hiddenRows = GetScrollableRowCount() - GetVisibleRowCount();
    if (hiddenRows <= 0)
    {
        return;
    }

    m_scrollOffset = std::max(0, std::min(m_scrollOffset + rows, hiddenRows));
}

bool SEASON3B::CWeeklyQuestWindow::UpdateMouseEvent()
{
    if (g_pNewUISystem->HandleFrameCornerClose(m_Pos, SEASON3B::INTERFACE_WEEKLY_QUESTS))
    {
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    if (m_BtnExit.UpdateMouseEvent())
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_WEEKLY_QUESTS);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    if (m_page == PAGE_DETAILS && m_BtnBack.UpdateMouseEvent())
    {
        ShowPage(PAGE_LIST);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    if (m_page == PAGE_LIST && UpdateListPageMouseEvent())
    {
        return false;
    }

    if (!CheckMouseIn(m_Pos.x, m_Pos.y, WINDOW_WIDTH, WindowHeight))
    {
        return true;
    }

    if (MouseWheel != 0)
    {
        // MouseWheel counts notches, one row per notch.
        Scroll(-MouseWheel);
        MouseWheel = 0;
    }

    return false;
}

bool SEASON3B::CWeeklyQuestWindow::UpdateListPageMouseEvent()
{
    for (int row = 0; row < VISIBLE_ROWS; ++row)
    {
        const auto index = m_scrollOffset + row;
        if (GetQuestAt(index) == nullptr)
        {
            break;
        }

        if (IsRowHovered(m_Pos.y + CONTENT_TOP + row * ROW_HEIGHT) && IsRelease(VK_LBUTTON))
        {
            PlayBuffer(SOUND_CLICK01);
            PickQuest(index);
            return true;
        }
    }

    return false;
}

bool SEASON3B::CWeeklyQuestWindow::UpdateKeyEvent()
{
    if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_WEEKLY_QUESTS))
    {
        return true;
    }

    if (IsPress(VK_ESCAPE))
    {
        // The first escape goes back to the list, the next one closes the window.
        if (m_page != PAGE_LIST)
        {
            ShowPage(PAGE_LIST);
        }
        else
        {
            g_pNewUISystem->Hide(SEASON3B::INTERFACE_WEEKLY_QUESTS);
        }

        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    if (IsPress(VK_DOWN))
    {
        Scroll(1);
        return false;
    }

    if (IsPress(VK_UP))
    {
        Scroll(-1);
        return false;
    }

    return true;
}

bool SEASON3B::CWeeklyQuestWindow::Update()
{
    if (m_page != PAGE_DETAILS || m_shownRevision == WeeklyQuests().GetRevision())
    {
        return true;
    }

    // The server replaced the list or updated the progress while the details are shown.
    if (GetSelectedQuest() == nullptr)
    {
        ShowPage(PAGE_LIST);
    }
    else
    {
        WrapDetailsOfSelected();
    }

    return true;
}

bool SEASON3B::CWeeklyQuestWindow::Render()
{
    EnableAlphaTest();
    glColor4f(1.f, 1.f, 1.f, 1.f);

    g_pRenderText->SetFont(g_hFont);
    UseTextColor(NormalColor);

    RenderBaseWindow();
    RenderTitle();

    if (m_page == PAGE_DETAILS)
    {
        RenderPanel(DETAIL_PANEL_HEIGHT);
        RenderDetailsPage();
        m_BtnBack.SetFont(g_hFont);
        m_BtnBack.Render();
    }
    else
    {
        RenderPanel(LIST_PANEL_HEIGHT);
        RenderListPage();
        RenderHint();
        RenderTimeUntilReset();
    }

    m_BtnExit.Render();
    DisableAlphaBlend();
    return true;
}

void SEASON3B::CWeeklyQuestWindow::RenderBaseWindow()
{
    const auto x = static_cast<float>(m_Pos.x);
    const auto y = static_cast<float>(m_Pos.y);
    const auto middleHeight = static_cast<float>(WindowHeight - FRAME_TOP_HEIGHT - FRAME_BOTTOM_HEIGHT);

    RenderImage(IMAGE_WEEKLYQUEST_BACK, x, y, float(WINDOW_WIDTH), float(WindowHeight));
    RenderImage(IMAGE_WEEKLYQUEST_TOP, x, y, float(WINDOW_WIDTH), float(FRAME_TOP_HEIGHT));

    // Like in the chat commands window, the side pieces are stretched instead of
    // drawn one to one, because the window is taller than they are.
    RenderImageStretch(IMAGE_WEEKLYQUEST_LEFT, x, y + float(FRAME_TOP_HEIGHT), float(FRAME_SIDE_WIDTH), middleHeight,
                       0.f, 0.f, float(FRAME_SIDE_WIDTH), float(FRAME_SIDE_TEXTURE_HEIGHT));
    RenderImageStretch(IMAGE_WEEKLYQUEST_RIGHT, x + float(WINDOW_WIDTH - FRAME_SIDE_WIDTH), y + float(FRAME_TOP_HEIGHT),
                       float(FRAME_SIDE_WIDTH), middleHeight, 0.f, 0.f, float(FRAME_SIDE_WIDTH),
                       float(FRAME_SIDE_TEXTURE_HEIGHT));

    RenderImage(IMAGE_WEEKLYQUEST_BOTTOM, x, y + float(WindowHeight - FRAME_BOTTOM_HEIGHT), float(WINDOW_WIDTH),
                float(FRAME_BOTTOM_HEIGHT));
}

void SEASON3B::CWeeklyQuestWindow::RenderTitle()
{
    // The name of a quest is shown as a heading of its details, so the frame keeps its title.
    g_pRenderText->SetFont(g_hFontBold);
    UseTextColor(TitleColor);
    RenderLine(m_Pos.x, m_Pos.y + TITLE_Y, I18N::Game::WeeklyQuestsTitle, WINDOW_WIDTH, 0, RT3_SORT_CENTER);
    g_pRenderText->SetFont(g_hFont);
}

void SEASON3B::CWeeklyQuestWindow::RenderListPage()
{
    const auto& quests = WeeklyQuests().GetQuests();
    if (quests.empty())
    {
        UseTextColor(NormalColor);
        RenderLine(m_Pos.x + CONTENT_LEFT, m_Pos.y + CONTENT_TOP, I18N::Game::WeeklyQuestsNone, CONTENT_WIDTH,
                   VISIBLE_ROWS * ROW_HEIGHT);
        return;
    }

    for (int row = 0; row < VISIBLE_ROWS; ++row)
    {
        const auto* quest = GetQuestAt(m_scrollOffset + row);
        if (quest == nullptr)
        {
            break;
        }

        RenderQuestRow(*quest, m_Pos.y + CONTENT_TOP + row * ROW_HEIGHT);
    }
}

void SEASON3B::CWeeklyQuestWindow::RenderQuestRow(const WeeklyQuest& quest, int y)
{
    const bool isHovered = IsRowHovered(y);
    if (isHovered)
    {
        RenderRowHighlight(y);
    }

    const auto nameWidth = CONTENT_WIDTH - PROGRESS_WIDTH - ARROW_WIDTH;
    const auto progressX = m_Pos.x + CONTENT_LEFT + nameWidth;
    const auto arrowX = progressX + PROGRESS_WIDTH;

    UseTextColor(isHovered ? HoveredColor : GetQuestColor(quest));
    RenderLine(m_Pos.x + CONTENT_LEFT, y, quest.Name.c_str(), nameWidth);

    const auto progress = GetProgressText(quest);
    RenderLine(progressX, y, progress.c_str(), PROGRESS_WIDTH, 0, RT3_SORT_RIGHT);

    // The arrow tells that there is more to see behind the row.
    UseTextColor(isHovered ? HoveredColor : ArrowColor);
    RenderLine(arrowX, y, ArrowMarker, ARROW_WIDTH, 0, RT3_SORT_RIGHT);
}

void SEASON3B::CWeeklyQuestWindow::RenderRowHighlight(int y)
{
    // RenderColor ignores the current color and draws white, so the color goes with the quad.
    RenderColorQuadARGB(static_cast<float>(m_Pos.x + CONTENT_LEFT), static_cast<float>(y),
                        static_cast<float>(CONTENT_WIDTH), static_cast<float>(ROW_HEIGHT), HoverBackgroundColor);
    EndRenderColor();
}

void SEASON3B::CWeeklyQuestWindow::RenderPanel(int height)
{
    RenderColorQuadARGB(static_cast<float>(m_Pos.x + PANEL_LEFT), static_cast<float>(m_Pos.y + PANEL_TOP),
                        static_cast<float>(PANEL_WIDTH), static_cast<float>(height), PanelBackdropColor);
    EndRenderColor();
}

void SEASON3B::CWeeklyQuestWindow::RenderHint()
{
    if (WeeklyQuests().GetQuests().empty())
    {
        return;
    }

    UseTextColor(HintColor);
    for (size_t line = 0; line < m_hintLines.size(); ++line)
    {
        const auto y = m_Pos.y + HINT_TOP + static_cast<int>(line) * ROW_HEIGHT;
        RenderLine(m_Pos.x + CONTENT_LEFT, y, m_hintLines[line].c_str(), CONTENT_WIDTH, 0, RT3_SORT_CENTER);
    }
}

void SEASON3B::CWeeklyQuestWindow::RenderDetailsPage()
{
    for (int row = 0; row < DETAIL_VISIBLE_ROWS; ++row)
    {
        const auto index = static_cast<size_t>(m_scrollOffset + row);
        if (index >= m_detailLines.size())
        {
            break;
        }

        const auto& line = m_detailLines[index];
        UseFont(line.Style);
        UseTextColor(GetColor(line.Style));
        RenderLine(m_Pos.x + CONTENT_LEFT, m_Pos.y + CONTENT_TOP + row * ROW_HEIGHT, line.Text.c_str(), CONTENT_WIDTH,
                   0, GetSort(line.Style));
    }

    g_pRenderText->SetFont(g_hFont);
}

void SEASON3B::CWeeklyQuestWindow::RenderTimeUntilReset()
{
    if (!WeeklyQuests().IsAvailable())
    {
        return;
    }

    UseTextColor(DescriptionColor);
    const auto text = FormatTimeUntilReset();
    RenderLine(m_Pos.x + CONTENT_LEFT, m_Pos.y + RESET_ROW_Y, text.c_str(), CONTENT_WIDTH, 0, RT3_SORT_CENTER);
}

void SEASON3B::CWeeklyQuestWindow::LoadImages()
{
    // The ids are shared with the other windows, but every window loads what it
    // draws - relying on another one having done it means an empty frame when
    // that window wasn't opened yet.
    LoadBitmap(L"Interface/newui_msgbox_back.jpg", IMAGE_WEEKLYQUEST_BACK, GL_LINEAR);
    LoadBitmap(L"Interface/newui_item_back04.tga", IMAGE_WEEKLYQUEST_TOP, GL_LINEAR);
    LoadBitmap(L"Interface/newui_item_back02-L.tga", IMAGE_WEEKLYQUEST_LEFT, GL_LINEAR);
    LoadBitmap(L"Interface/newui_item_back02-R.tga", IMAGE_WEEKLYQUEST_RIGHT, GL_LINEAR);
    LoadBitmap(L"Interface/newui_item_back03.tga", IMAGE_WEEKLYQUEST_BOTTOM, GL_LINEAR);
    LoadBitmap(L"Interface/newui_exit_00.tga", IMAGE_WEEKLYQUEST_BTN_EXIT, GL_LINEAR);
    LoadBitmap(L"Interface/newui_btn_empty_small.tga", IMAGE_WEEKLYQUEST_BTN, GL_LINEAR);
}

void SEASON3B::CWeeklyQuestWindow::UnloadImages()
{
    DeleteBitmap(IMAGE_WEEKLYQUEST_BACK);
    DeleteBitmap(IMAGE_WEEKLYQUEST_TOP);
    DeleteBitmap(IMAGE_WEEKLYQUEST_LEFT);
    DeleteBitmap(IMAGE_WEEKLYQUEST_RIGHT);
    DeleteBitmap(IMAGE_WEEKLYQUEST_BOTTOM);
    DeleteBitmap(IMAGE_WEEKLYQUEST_BTN_EXIT);
    DeleteBitmap(IMAGE_WEEKLYQUEST_BTN);
}
