// NewUIQuestTracker.cpp: implementation of the CNewUIQuestTracker class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "I18N/All.h"

#include "UI/NewUI/Quests/NewUIQuestTracker.h"

#include "Audio/DSPlaySound.h"
#include "Engine/Object/ZzzInventory.h"
#include "GameLogic/Quests/WeeklyQuestCatalog.h"
#include "UI/NewUI/NewUISystem.h"

using namespace SEASON3B;
using GameLogic::Quests::WeeklyQuests;

namespace
{
// The party list takes this much at the right of the free screen, see CNewUIPartyListWindow::SetPos.
constexpr int PartyListWidth = 77 + 2;
constexpr const wchar_t* Ellipsis = L"...";
// The dark panel behind the text, so that it can be read on any map.
constexpr unsigned int BackgroundColor = 0x8C000000u;

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
    Show(true);
    return true;
}

void CNewUIQuestTracker::Release()
{
    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = nullptr;
    }
}

int CNewUIQuestTracker::GetLeft() const
{
    // Without a party it stays at the right edge, behind the windows which open there.
    const int right = PartyNumber > 0 ? GetScreenWidth() - PartyListWidth - TRACKER_PARTY_GAP : REFERENCE_WIDTH - 2;
    return right - TRACKER_WIDTH;
}

int CNewUIQuestTracker::GetHeight() const
{
    return static_cast<int>(m_lines.size()) * TRACKER_LINE_HEIGHT + 2 * TRACKER_PADDING;
}

bool CNewUIQuestTracker::UpdateMouseEvent()
{
    if (m_lines.empty() || !CheckMouseIn(GetLeft(), TRACKER_TOP, TRACKER_WIDTH, GetHeight()))
    {
        return true;
    }

    if (IsRelease(VK_LBUTTON) && !g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_WEEKLY_QUESTS))
    {
        g_pNewUISystem->Show(SEASON3B::INTERFACE_WEEKLY_QUESTS);
        PlayBuffer(SOUND_CLICK01);
    }

    // A click on the tracker doesn't move the character.
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
        m_isLaidOut = false;
    }

    return true;
}

void CNewUIQuestTracker::AddLine(const std::wstring& text, BYTE red, BYTE green, BYTE blue, bool isBold)
{
    g_pRenderText->SetFont(isBold ? g_hFontBold : g_hFont);
    m_lines.push_back({Fit(text, TRACKER_WIDTH - 2 * TRACKER_PADDING), red, green, blue, isBold});
}

void CNewUIQuestTracker::Layout()
{
    m_lines.clear();
    m_layoutRevision = WeeklyQuests().GetRevision();
    m_isLaidOut = true;
    if (!WeeklyQuests().IsAvailable())
    {
        return;
    }

    const auto content = UI::Quests::BuildQuestTracker(WeeklyQuests().GetQuests(), TRACKER_MAX_QUESTS);
    if (content.IsEmpty())
    {
        return;
    }

    AddLine(content.Chapter.empty() ? std::wstring(I18N::Game::QuestTrackerTitle) : content.Chapter, 255, 220, 120, true);
    for (const auto& quest : content.Quests)
    {
        AddLine(quest.Name, 36, 242, 252, false);
        if (quest.IsRewardPending)
        {
            AddLine(I18N::Game::QuestTrackerRewardPending, 255, 190, 100, false);
        }
        else if (!quest.Step.empty())
        {
            AddLine(L" " + quest.Step, 220, 220, 220, false);
        }
    }
}

bool CNewUIQuestTracker::Render()
{
    if (!m_isLaidOut)
    {
        Layout();
    }

    if (m_lines.empty())
    {
        return true;
    }

    EnableAlphaTest();
    const int left = GetLeft();
    RenderColorQuadARGB(static_cast<float>(left), static_cast<float>(TRACKER_TOP), static_cast<float>(TRACKER_WIDTH),
                        static_cast<float>(GetHeight()), BackgroundColor);

    g_pRenderText->SetBgColor(0);
    int y = TRACKER_TOP + TRACKER_PADDING;
    for (const auto& line : m_lines)
    {
        g_pRenderText->SetFont(line.IsBold ? g_hFontBold : g_hFont);
        g_pRenderText->SetTextColor(line.Red, line.Green, line.Blue, 255);
        g_pRenderText->RenderText(left + TRACKER_PADDING, y, line.Text.c_str(), TRACKER_WIDTH - 2 * TRACKER_PADDING, 0, RT3_SORT_LEFT);
        y += TRACKER_LINE_HEIGHT;
    }

    g_pRenderText->SetFont(g_hFont);
    DisableAlphaBlend();
    return true;
}

float CNewUIQuestTracker::GetLayerDepth()
{
    // Below the windows which open at the right, like the inventory (4.2).
    return 1.0f;
}
