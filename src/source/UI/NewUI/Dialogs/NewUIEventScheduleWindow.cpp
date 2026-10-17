// NewUIEventScheduleWindow.cpp: implementation of the CNewUIEventScheduleWindow class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "I18N/All.h"

#include "UI/NewUI/Dialogs/NewUIEventScheduleWindow.h"

#include "Audio/DSPlaySound.h"
#include "GameLogic/Events/EventScheduleCatalog.h"
#include "UI/NewUI/NewUISystem.h"

#include <algorithm>
#include <string>

using namespace SEASON3B;
using GameLogic::Events::EventSchedule;
using GameLogic::Events::EventScheduleCatalog;
using GameLogic::Events::ScheduledEvent;

namespace
{
struct TextColor
{
    BYTE Red;
    BYTE Green;
    BYTE Blue;
};

constexpr TextColor TitleColor = {255, 220, 120};
constexpr TextColor NameColor = {255, 255, 255};
constexpr TextColor NormalColor = {200, 200, 200};
constexpr TextColor SoonColor = {255, 190, 90};
constexpr TextColor RunningColor = {120, 230, 120};

// An event which starts in less than this is shown in the color of soon events.
constexpr int SoonSeconds = 15 * 60;

void UseTextColor(const TextColor& color)
{
    g_pRenderText->SetTextColor(color.Red, color.Green, color.Blue, 255);
    g_pRenderText->SetBgColor(0);
}

// An empty text has to be skipped: the text renderer would leave a stray glyph behind.
void RenderLine(int x, int y, const wchar_t* text, int boxWidth, int sort = RT3_SORT_LEFT)
{
    if (text == nullptr || text[0] == L'\0')
    {
        return;
    }

    g_pRenderText->RenderText(x, y, text, boxWidth, 0, sort);
}

// "08:30".
std::wstring FormatClock(int minuteOfDay)
{
    wchar_t text[16] = {};
    mu_swprintf_s(text, L"%02d:%02d", minuteOfDay / 60, minuteOfDay % 60);
    return text;
}

// "1 h 05 min" or "25 min"; a started minute counts as a whole one.
std::wstring FormatDuration(int seconds)
{
    const auto minutes = std::max(1, (seconds + 59) / 60);
    wchar_t text[32] = {};
    if (minutes >= 60)
    {
        mu_swprintf_s(text, L"%d h %02d min", minutes / 60, minutes % 60);
    }
    else
    {
        mu_swprintf_s(text, L"%d min", minutes);
    }

    return text;
}
} // namespace

SEASON3B::CNewUIEventScheduleWindow::CNewUIEventScheduleWindow()
{
    m_pNewUIMng = nullptr;
    m_Pos.x = 0;
    m_Pos.y = 0;
    m_scrollOffset = 0;
}

SEASON3B::CNewUIEventScheduleWindow::~CNewUIEventScheduleWindow()
{
    Release();
}

bool SEASON3B::CNewUIEventScheduleWindow::Create(CNewUIManager* pNewUIMng, int x, int y)
{
    if (pNewUIMng == nullptr)
    {
        return false;
    }

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_EVENT_SCHEDULE, this);

    LoadImages();
    SetPos(x, y);

    wchar_t closeText[256] = {};
    mu_swprintf_s(closeText, I18N::Game::CloseS, L"H");
    m_BtnExit.ChangeButtonImgState(true, IMAGE_EVENTSCHEDULE_BTN_EXIT);
    m_BtnExit.ChangeToolTipText(closeText, true);

    Show(false);
    return true;
}

void SEASON3B::CNewUIEventScheduleWindow::Release()
{
    UnloadImages();

    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = nullptr;
    }
}

void SEASON3B::CNewUIEventScheduleWindow::SetPos(int x, int y)
{
    m_Pos.x = x;
    m_Pos.y = y;
    m_BtnExit.ChangeButtonInfo(m_Pos.x + EXIT_BUTTON_X, m_Pos.y + EXIT_BUTTON_Y, EXIT_BUTTON_WIDTH, EXIT_BUTTON_HEIGHT);
}

float SEASON3B::CNewUIEventScheduleWindow::GetLayerDepth()
{
    return LayerDepth;
}

float SEASON3B::CNewUIEventScheduleWindow::GetKeyEventOrder()
{
    return 10.f;
}

void SEASON3B::CNewUIEventScheduleWindow::OpenningProcess()
{
    m_scrollOffset = 0;
}

void SEASON3B::CNewUIEventScheduleWindow::ClosingProcess()
{
}

int SEASON3B::CNewUIEventScheduleWindow::GetHiddenEventCount() const
{
    return std::max(0, static_cast<int>(EventSchedule().GetEvents().size()) - VISIBLE_EVENTS);
}

bool SEASON3B::CNewUIEventScheduleWindow::UpdateMouseEvent()
{
    if (g_pNewUISystem->HandleFrameCornerClose(m_Pos, SEASON3B::INTERFACE_EVENT_SCHEDULE))
    {
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    if (m_BtnExit.UpdateMouseEvent())
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_EVENT_SCHEDULE);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    if (!CheckMouseIn(m_Pos.x, m_Pos.y, WINDOW_WIDTH, WindowHeight))
    {
        return true;
    }

    const auto hiddenEvents = GetHiddenEventCount();
    if (MouseWheel != 0 && hiddenEvents > 0)
    {
        // MouseWheel counts notches, one event per notch.
        m_scrollOffset = std::max(0, std::min(m_scrollOffset - MouseWheel, hiddenEvents));
        MouseWheel = 0;
    }

    return false;
}

bool SEASON3B::CNewUIEventScheduleWindow::UpdateKeyEvent()
{
    if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_EVENT_SCHEDULE))
    {
        return true;
    }

    if (IsPress(VK_ESCAPE))
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_EVENT_SCHEDULE);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    if (IsPress(VK_DOWN) && m_scrollOffset < GetHiddenEventCount())
    {
        ++m_scrollOffset;
        return false;
    }

    if (IsPress(VK_UP) && m_scrollOffset > 0)
    {
        --m_scrollOffset;
        return false;
    }

    return true;
}

bool SEASON3B::CNewUIEventScheduleWindow::Update()
{
    return true;
}

bool SEASON3B::CNewUIEventScheduleWindow::Render()
{
    EnableAlphaTest();
    glColor4f(1.f, 1.f, 1.f, 1.f);

    RenderBaseWindow();

    g_pRenderText->SetFont(g_hFontBold);
    UseTextColor(TitleColor);
    RenderLine(m_Pos.x, m_Pos.y + TITLE_Y, I18N::Game::EventScheduleTitle, WINDOW_WIDTH, RT3_SORT_CENTER);

    // The window is redrawn every frame, so the times count down by themselves.
    const auto serverSecondOfDay = EventSchedule().GetServerSecondOfDay(GetTickCount64());
    RenderEvents(serverSecondOfDay);
    RenderServerTime(serverSecondOfDay);

    m_BtnExit.Render();
    DisableAlphaBlend();
    return true;
}

void SEASON3B::CNewUIEventScheduleWindow::RenderBaseWindow()
{
    const auto x = static_cast<float>(m_Pos.x);
    const auto y = static_cast<float>(m_Pos.y);
    const auto middleHeight = static_cast<float>(WindowHeight - FRAME_TOP_HEIGHT - FRAME_BOTTOM_HEIGHT);

    RenderImage(IMAGE_EVENTSCHEDULE_BACK, x, y, float(WINDOW_WIDTH), float(WindowHeight));
    RenderImage(IMAGE_EVENTSCHEDULE_TOP, x, y, float(WINDOW_WIDTH), float(FRAME_TOP_HEIGHT));
    RenderImageStretch(IMAGE_EVENTSCHEDULE_LEFT, x, y + float(FRAME_TOP_HEIGHT), float(FRAME_SIDE_WIDTH), middleHeight,
                       0.f, 0.f, float(FRAME_SIDE_WIDTH), float(FRAME_SIDE_TEXTURE_HEIGHT));
    RenderImageStretch(IMAGE_EVENTSCHEDULE_RIGHT, x + float(WINDOW_WIDTH - FRAME_SIDE_WIDTH), y + float(FRAME_TOP_HEIGHT),
                       float(FRAME_SIDE_WIDTH), middleHeight, 0.f, 0.f, float(FRAME_SIDE_WIDTH),
                       float(FRAME_SIDE_TEXTURE_HEIGHT));
    RenderImage(IMAGE_EVENTSCHEDULE_BOTTOM, x, y + float(WindowHeight - FRAME_BOTTOM_HEIGHT), float(WINDOW_WIDTH),
                float(FRAME_BOTTOM_HEIGHT));
}

void SEASON3B::CNewUIEventScheduleWindow::RenderEvents(int serverSecondOfDay)
{
    const auto& events = EventSchedule().GetEvents();
    const auto left = m_Pos.x + CONTENT_LEFT;
    if (events.empty())
    {
        g_pRenderText->SetFont(g_hFont);
        UseTextColor(NormalColor);
        g_pRenderText->RenderText(left, m_Pos.y + CONTENT_TOP, I18N::Game::EventScheduleNone, CONTENT_WIDTH,
                                  VISIBLE_EVENTS * EVENT_HEIGHT, RT3_SORT_LEFT);
        return;
    }

    m_scrollOffset = std::min(m_scrollOffset, GetHiddenEventCount());
    const auto last = std::min(static_cast<int>(events.size()), m_scrollOffset + VISIBLE_EVENTS);
    auto y = m_Pos.y + CONTENT_TOP;
    for (int i = m_scrollOffset; i < last; ++i, y += EVENT_HEIGHT)
    {
        const auto& scheduledEvent = events[i];
        const auto state = EventScheduleCatalog::GetState(scheduledEvent, serverSecondOfDay);

        g_pRenderText->SetFont(g_hFontBold);
        UseTextColor(NameColor);
        RenderLine(left, y, scheduledEvent.Name.c_str(), CONTENT_WIDTH);

        wchar_t stateText[128] = {};
        const auto duration = FormatDuration(state.SecondsLeft);
        if (state.IsRunning)
        {
            mu_swprintf_s(stateText, I18N::Game::EventScheduleRunning, duration.c_str());
            UseTextColor(RunningColor);
        }
        else
        {
            const auto clock = FormatClock(state.StartMinute);
            mu_swprintf_s(stateText, I18N::Game::EventScheduleNextStart, clock.c_str(), duration.c_str());
            UseTextColor(state.SecondsLeft <= SoonSeconds ? SoonColor : NormalColor);
        }

        g_pRenderText->SetFont(g_hFont);
        RenderLine(left, y + ROW_HEIGHT, stateText, CONTENT_WIDTH);
    }
}

void SEASON3B::CNewUIEventScheduleWindow::RenderServerTime(int serverSecondOfDay)
{
    wchar_t text[96] = {};
    const auto clock = FormatClock(serverSecondOfDay / 60);
    const auto& timeZoneName = EventSchedule().GetTimeZoneName();
    if (timeZoneName.empty())
    {
        mu_swprintf_s(text, I18N::Game::EventScheduleServerTime, clock.c_str());
    }
    else
    {
        mu_swprintf_s(text, I18N::Game::EventScheduleZoneTime, timeZoneName.c_str(), clock.c_str());
    }

    g_pRenderText->SetFont(g_hFont);
    UseTextColor(NormalColor);
    RenderLine(m_Pos.x, m_Pos.y + SERVER_TIME_Y, text, WINDOW_WIDTH, RT3_SORT_CENTER);
}

void SEASON3B::CNewUIEventScheduleWindow::LoadImages()
{
    // The ids are shared with the other windows, but every window loads what it draws.
    LoadBitmap(L"Interface/newui_msgbox_back.jpg", IMAGE_EVENTSCHEDULE_BACK, GL_LINEAR);
    LoadBitmap(L"Interface/newui_item_back01.tga", IMAGE_EVENTSCHEDULE_TOP, GL_LINEAR);
    LoadBitmap(L"Interface/newui_item_back02-L.tga", IMAGE_EVENTSCHEDULE_LEFT, GL_LINEAR);
    LoadBitmap(L"Interface/newui_item_back02-R.tga", IMAGE_EVENTSCHEDULE_RIGHT, GL_LINEAR);
    LoadBitmap(L"Interface/newui_item_back03.tga", IMAGE_EVENTSCHEDULE_BOTTOM, GL_LINEAR);
    LoadBitmap(L"Interface/newui_exit_00.tga", IMAGE_EVENTSCHEDULE_BTN_EXIT, GL_LINEAR);
}

void SEASON3B::CNewUIEventScheduleWindow::UnloadImages()
{
    DeleteBitmap(IMAGE_EVENTSCHEDULE_BACK);
    DeleteBitmap(IMAGE_EVENTSCHEDULE_TOP);
    DeleteBitmap(IMAGE_EVENTSCHEDULE_LEFT);
    DeleteBitmap(IMAGE_EVENTSCHEDULE_RIGHT);
    DeleteBitmap(IMAGE_EVENTSCHEDULE_BOTTOM);
    DeleteBitmap(IMAGE_EVENTSCHEDULE_BTN_EXIT);
}
