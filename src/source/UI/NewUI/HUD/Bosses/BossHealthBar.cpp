#include "stdafx.h"

#include "UI/NewUI/HUD/Bosses/BossHealthBar.h"

#include "App/Platform/Windows/Winmain.h" // g_hFont, g_hFontBold
#include "GameLogic/Monsters/BossHealthState.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "UI/Legacy/UIControls.h" // g_pRenderText

#include <string>

namespace UI::Bosses::HealthBar
{
namespace
{
using GameLogic::Monsters::BossHealth;

// In the reference coordinates of 640 x 480, below the name of the map.
constexpr float CenterX = 320.f;
constexpr float BarWidth = 220.f;
constexpr float BarHeight = 7.f;
constexpr float BarTop = 52.f;
constexpr float Border = 1.f;
constexpr int TitleTop = 39;

constexpr float MaximumPercent = 100.f;

constexpr unsigned int BorderColor = 0xFF8A6A2Au;
constexpr unsigned int BackgroundColor = 0xC0100808u;
constexpr unsigned int HealthColor = 0xFFC0261Cu;

constexpr BYTE TitleRed = 255;
constexpr BYTE TitleGreen = 220;
constexpr BYTE TitleBlue = 150;
constexpr BYTE Opaque = 255;

// The title only changes with the health or the phase, so it's not built in each frame.
struct CachedTitle
{
    int Key = -1;
    int HealthPercent = -1;
    std::wstring PhaseName;
    std::wstring Text;
};

CachedTitle s_title;

const std::wstring& GetTitle(const BossHealth& boss)
{
    if (s_title.Key == boss.Key && s_title.HealthPercent == boss.HealthPercent && s_title.PhaseName == boss.PhaseName)
    {
        return s_title.Text;
    }

    s_title.Key = boss.Key;
    s_title.HealthPercent = boss.HealthPercent;
    s_title.PhaseName = boss.PhaseName;
    s_title.Text = boss.BossName;
    if (!boss.PhaseName.empty())
    {
        s_title.Text += L" - " + boss.PhaseName;
    }

    s_title.Text += L"  " + std::to_wstring(boss.HealthPercent) + L" %";
    return s_title.Text;
}

void RenderBar(const BossHealth& boss)
{
    const float left = CenterX - BarWidth / 2.f;
    const float filledWidth = BarWidth * static_cast<float>(boss.HealthPercent) / MaximumPercent;

    EnableAlphaTest();
    RenderColorQuadARGB(left - Border, BarTop - Border, BarWidth + Border * 2.f, BarHeight + Border * 2.f, BorderColor);
    RenderColorQuadARGB(left, BarTop, BarWidth, BarHeight, BackgroundColor);
    if (filledWidth > 0.f)
    {
        RenderColorQuadARGB(left, BarTop, filledWidth, BarHeight, HealthColor);
    }
}

void RenderTitle(const BossHealth& boss)
{
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(TitleRed, TitleGreen, TitleBlue, Opaque);
    g_pRenderText->RenderText(static_cast<int>(CenterX), TitleTop, GetTitle(boss).c_str(), 0, 0, RT3_WRITE_CENTER);
    g_pRenderText->SetFont(g_hFont);
}
} // namespace

void Render()
{
    const BossHealth* boss = GameLogic::Monsters::BossHealthBar().Current();
    if (boss == nullptr)
    {
        return;
    }

    RenderBar(*boss);
    RenderTitle(*boss);
}
} // namespace UI::Bosses::HealthBar
