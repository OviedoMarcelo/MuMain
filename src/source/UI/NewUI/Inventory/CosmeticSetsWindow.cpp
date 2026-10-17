// CosmeticSetsWindow.cpp: implementation of the CCosmeticSetsWindow class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "I18N/All.h"

#include "UI/NewUI/Inventory/CosmeticSetsWindow.h"

#include "Audio/DSPlaySound.h"
#include "Core/Text/TextLineWrap.h"
#include "Engine/Object/ZzzInventory.h"
#include "GameLogic/Commands/ChatCommandCatalog.h"
#include "UI/NewUI/NewUISystem.h"

#include <algorithm>
#include <string>

using namespace SEASON3B;
using GameLogic::Cosmetics::CosmeticSet;
using GameLogic::Cosmetics::CosmeticSets;

namespace
{
// The chat command which wears a set, or takes it off with "off".
constexpr const wchar_t* WearCommand = L"/cosmetico ";
constexpr const wchar_t* TakeOffArgument = L"off";

// The item group of the armors, whose piece shows the set in the list.
constexpr int ArmorGroup = 8;

struct TextColor
{
    BYTE Red;
    BYTE Green;
    BYTE Blue;
};

constexpr TextColor TitleColor = {255, 220, 120};
constexpr TextColor NormalColor = {220, 220, 220};
constexpr TextColor WornColor = {150, 230, 150};
constexpr TextColor LockedColor = {150, 150, 150};
constexpr TextColor HintColor = {240, 225, 180};
constexpr TextColor HoveredColor = {255, 255, 0};

// The backgrounds of the rows, as ARGB: the one under the mouse and the picked one.
constexpr unsigned int HoverBackgroundColor = 0x46C8A03Cu;
constexpr unsigned int SelectedBackgroundColor = 0x6E3C78C8u;
// The dark panel behind the content, the one of the boxes of the pet window.
constexpr unsigned int PanelBackdropColor = 0x99000000u;

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

int MeasureInReferenceUnits(const wchar_t* text, size_t length)
{
    return g_pRenderText->MeasureText(text, static_cast<int>(length)).cx;
}

const wchar_t* GetStateText(const CosmeticSet& set, bool isWorn)
{
    if (isWorn)
    {
        return I18N::Game::CosmeticSetsWorn;
    }

    if (!set.IsUnlocked)
    {
        return I18N::Game::CosmeticSetsLocked;
    }

    return set.CanWear ? L"" : I18N::Game::CosmeticSetsOtherClass;
}

const TextColor& GetNameColor(const CosmeticSet& set, bool isWorn)
{
    if (isWorn)
    {
        return WornColor;
    }

    return set.IsUnlocked && set.CanWear ? NormalColor : LockedColor;
}

bool IsWorn(const CosmeticSet& set)
{
    const auto* worn = CosmeticSets().GetWornSet();
    return worn != nullptr && worn->Number == set.Number;
}
} // namespace

SEASON3B::CCosmeticSetsWindow::CCosmeticSetsWindow()
{
    m_pNewUIMng = nullptr;
    m_pNewUI3DRenderMng = nullptr;
    m_Pos.x = 0;
    m_Pos.y = 0;
    m_scrollOffset = 0;
    m_selectedSetNumber = GameLogic::Cosmetics::CosmeticSetCatalog::NoSet;
}

SEASON3B::CCosmeticSetsWindow::~CCosmeticSetsWindow()
{
    Release();
}

bool SEASON3B::CCosmeticSetsWindow::Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng, int x, int y)
{
    if (pNewUIMng == nullptr || pNewUI3DRenderMng == nullptr)
    {
        return false;
    }

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_COSMETIC_SETS, this);

    m_pNewUI3DRenderMng = pNewUI3DRenderMng;
    m_pNewUI3DRenderMng->Add3DRenderObj(this, INVENTORY_CAMERA_Z_ORDER);

    LoadImages();
    SetPos(x, y);
    InitButtons();
    Show(false);

    return true;
}

void SEASON3B::CCosmeticSetsWindow::Release()
{
    UnloadImages();

    if (m_pNewUI3DRenderMng)
    {
        m_pNewUI3DRenderMng->Remove3DRenderObj(this);
        m_pNewUI3DRenderMng = nullptr;
    }

    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = nullptr;
    }
}

void SEASON3B::CCosmeticSetsWindow::SetPos(int x, int y)
{
    m_Pos.x = x;
    m_Pos.y = y;

    m_BtnExit.ChangeButtonInfo(m_Pos.x + EXIT_BUTTON_X, m_Pos.y + EXIT_BUTTON_Y, EXIT_BUTTON_WIDTH, EXIT_BUTTON_HEIGHT);
    m_BtnWear.ChangeButtonInfo(m_Pos.x + CONTENT_LEFT, m_Pos.y + BUTTON_ROW_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
    m_BtnTakeOff.ChangeButtonInfo(m_Pos.x + WINDOW_WIDTH - CONTENT_LEFT - BUTTON_WIDTH, m_Pos.y + BUTTON_ROW_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
}

void SEASON3B::CCosmeticSetsWindow::InitButtons()
{
    m_BtnExit.ChangeButtonImgState(true, IMAGE_COSMETICSETS_BTN_EXIT);
    m_BtnExit.ChangeToolTipText(&I18N::Game::Close388, true);

    m_BtnWear.ChangeButtonImgState(true, IMAGE_COSMETICSETS_BTN, true);
    m_BtnWear.ChangeText(&I18N::Game::CosmeticSetsWear);

    m_BtnTakeOff.ChangeButtonImgState(true, IMAGE_COSMETICSETS_BTN, true);
    m_BtnTakeOff.ChangeText(&I18N::Game::CosmeticSetsTakeOff);
}

float SEASON3B::CCosmeticSetsWindow::GetLayerDepth()
{
    return LayerDepth;
}

float SEASON3B::CCosmeticSetsWindow::GetKeyEventOrder()
{
    return 10.f;
}

bool SEASON3B::CCosmeticSetsWindow::IsVisible() const
{
    return CNewUIObj::IsVisible();
}

void SEASON3B::CCosmeticSetsWindow::OpenningProcess()
{
    m_scrollOffset = 0;
    const auto* worn = CosmeticSets().GetWornSet();
    m_selectedSetNumber = worn != nullptr ? worn->Number : GameLogic::Cosmetics::CosmeticSetCatalog::NoSet;
    WrapHint();
}

void SEASON3B::CCosmeticSetsWindow::ClosingProcess()
{
    m_hintLines.clear();
}

void SEASON3B::CCosmeticSetsWindow::WrapHint()
{
    g_pRenderText->SetFont(g_hFont);
    m_hintLines = WrapTextToWidth(I18N::Game::CosmeticSetsHint, CONTENT_WIDTH, MeasureInReferenceUnits);
    if (m_hintLines.size() > HINT_LINES)
    {
        m_hintLines.resize(HINT_LINES);
    }
}

const CosmeticSet* SEASON3B::CCosmeticSetsWindow::GetSetAt(int row) const
{
    const auto& sets = CosmeticSets().GetSets();
    if (row < 0 || static_cast<size_t>(row) >= sets.size())
    {
        return nullptr;
    }

    return &sets[row];
}

const CosmeticSet* SEASON3B::CCosmeticSetsWindow::GetSelectedSet() const
{
    const auto& sets = CosmeticSets().GetSets();
    const auto selected = std::find_if(sets.begin(), sets.end(),
                                       [this](const CosmeticSet& set) { return set.Number == m_selectedSetNumber; });
    return selected == sets.end() ? nullptr : &*selected;
}

bool SEASON3B::CCosmeticSetsWindow::CanWearSelected() const
{
    const auto* set = GetSelectedSet();
    return set != nullptr && set->IsUnlocked && set->CanWear && !IsWorn(*set);
}

int SEASON3B::CCosmeticSetsWindow::GetRowY(int row) const
{
    return m_Pos.y + CONTENT_TOP + row * ROW_HEIGHT;
}

bool SEASON3B::CCosmeticSetsWindow::IsRowHovered(int row) const
{
    return CheckMouseIn(m_Pos.x + CONTENT_LEFT, GetRowY(row), CONTENT_WIDTH, ROW_HEIGHT);
}

void SEASON3B::CCosmeticSetsWindow::Scroll(int rows)
{
    const auto hiddenRows = static_cast<int>(CosmeticSets().GetSets().size()) - VISIBLE_ROWS;
    if (hiddenRows <= 0)
    {
        m_scrollOffset = 0;
        return;
    }

    m_scrollOffset = std::clamp(m_scrollOffset + rows, 0, hiddenRows);
}

bool SEASON3B::CCosmeticSetsWindow::UpdateMouseEvent()
{
    if (g_pNewUISystem->HandleFrameCornerClose(m_Pos, SEASON3B::INTERFACE_COSMETIC_SETS))
    {
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    if (m_BtnExit.UpdateMouseEvent())
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_COSMETIC_SETS);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    if (UpdateButtonsMouseEvent() || UpdateListMouseEvent())
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

bool SEASON3B::CCosmeticSetsWindow::UpdateButtonsMouseEvent()
{
    // The server answers with a new CosmeticSets message, which updates the list and the armor.
    if (CanWearSelected() && m_BtnWear.UpdateMouseEvent())
    {
        PlayBuffer(SOUND_CLICK01);
        const auto command = std::wstring(WearCommand) + std::to_wstring(m_selectedSetNumber);
        GameLogic::Commands::ChatCommandCatalog::Execute(command);
        return true;
    }

    if (CosmeticSets().GetWornSet() != nullptr && m_BtnTakeOff.UpdateMouseEvent())
    {
        PlayBuffer(SOUND_CLICK01);
        const auto command = std::wstring(WearCommand) + TakeOffArgument;
        GameLogic::Commands::ChatCommandCatalog::Execute(command);
        return true;
    }

    return false;
}

bool SEASON3B::CCosmeticSetsWindow::UpdateListMouseEvent()
{
    for (int row = 0; row < VISIBLE_ROWS; ++row)
    {
        const auto* set = GetSetAt(m_scrollOffset + row);
        if (set == nullptr)
        {
            break;
        }

        if (IsRowHovered(row) && IsRelease(VK_LBUTTON))
        {
            PlayBuffer(SOUND_CLICK01);
            m_selectedSetNumber = set->Number;
            return true;
        }
    }

    return false;
}

bool SEASON3B::CCosmeticSetsWindow::UpdateKeyEvent()
{
    if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_COSMETIC_SETS))
    {
        return true;
    }

    if (IsPress(VK_ESCAPE))
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_COSMETIC_SETS);
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

bool SEASON3B::CCosmeticSetsWindow::Update()
{
    // The list may have become shorter, e.g. after the configuration changed.
    Scroll(0);
    return true;
}

bool SEASON3B::CCosmeticSetsWindow::Render()
{
    EnableAlphaTest();
    glColor4f(1.f, 1.f, 1.f, 1.f);

    g_pRenderText->SetFont(g_hFont);
    UseTextColor(NormalColor);

    RenderBaseWindow();
    RenderTitle();
    RenderPanel();
    RenderList();
    RenderHint();
    RenderButtons();

    m_BtnExit.Render();
    DisableAlphaBlend();
    return true;
}

void SEASON3B::CCosmeticSetsWindow::Render3D()
{
    for (int row = 0; row < VISIBLE_ROWS; ++row)
    {
        const auto* set = GetSetAt(m_scrollOffset + row);
        if (set == nullptr)
        {
            break;
        }

        const int itemType = static_cast<int>(ITEMINDEX(ArmorGroup, set->ItemNumber));
        const auto x = static_cast<float>(m_Pos.x + CONTENT_LEFT);
        const auto y = static_cast<float>(GetRowY(row) + (ROW_HEIGHT - ICON_HEIGHT) / 2);
        ::RenderItem3D(x, y, static_cast<float>(ICON_WIDTH), static_cast<float>(ICON_HEIGHT), itemType, set->Level,
                       set->IsExcellent ? 1 : 0, 0, false);
    }
}

void SEASON3B::CCosmeticSetsWindow::RenderBaseWindow()
{
    const auto x = static_cast<float>(m_Pos.x);
    const auto y = static_cast<float>(m_Pos.y);
    const auto middleHeight = static_cast<float>(WindowHeight - FRAME_TOP_HEIGHT - FRAME_BOTTOM_HEIGHT);

    RenderImage(IMAGE_COSMETICSETS_BACK, x, y, float(WINDOW_WIDTH), float(WindowHeight));
    RenderImage(IMAGE_COSMETICSETS_TOP, x, y, float(WINDOW_WIDTH), float(FRAME_TOP_HEIGHT));

    // Like in the quests window, the side pieces are stretched, because the window is taller than they are.
    RenderImageStretch(IMAGE_COSMETICSETS_LEFT, x, y + float(FRAME_TOP_HEIGHT), float(FRAME_SIDE_WIDTH), middleHeight,
                       0.f, 0.f, float(FRAME_SIDE_WIDTH), float(FRAME_SIDE_TEXTURE_HEIGHT));
    RenderImageStretch(IMAGE_COSMETICSETS_RIGHT, x + float(WINDOW_WIDTH - FRAME_SIDE_WIDTH), y + float(FRAME_TOP_HEIGHT),
                       float(FRAME_SIDE_WIDTH), middleHeight, 0.f, 0.f, float(FRAME_SIDE_WIDTH),
                       float(FRAME_SIDE_TEXTURE_HEIGHT));

    RenderImage(IMAGE_COSMETICSETS_BOTTOM, x, y + float(WindowHeight - FRAME_BOTTOM_HEIGHT), float(WINDOW_WIDTH),
                float(FRAME_BOTTOM_HEIGHT));
}

void SEASON3B::CCosmeticSetsWindow::RenderTitle()
{
    g_pRenderText->SetFont(g_hFontBold);
    UseTextColor(TitleColor);
    RenderLine(m_Pos.x, m_Pos.y + TITLE_Y, I18N::Game::CosmeticSetsTitle, WINDOW_WIDTH, RT3_SORT_CENTER);
    g_pRenderText->SetFont(g_hFont);
}

void SEASON3B::CCosmeticSetsWindow::RenderPanel()
{
    RenderColorQuadARGB(static_cast<float>(m_Pos.x + PANEL_LEFT), static_cast<float>(m_Pos.y + PANEL_TOP),
                        static_cast<float>(PANEL_WIDTH), static_cast<float>(PANEL_HEIGHT), PanelBackdropColor);
    EndRenderColor();
}

void SEASON3B::CCosmeticSetsWindow::RenderList()
{
    if (CosmeticSets().GetSets().empty())
    {
        UseTextColor(NormalColor);
        RenderLine(m_Pos.x + CONTENT_LEFT, m_Pos.y + CONTENT_TOP, I18N::Game::CosmeticSetsNone, CONTENT_WIDTH);
        return;
    }

    for (int row = 0; row < VISIBLE_ROWS; ++row)
    {
        const auto* set = GetSetAt(m_scrollOffset + row);
        if (set == nullptr)
        {
            break;
        }

        RenderSetRow(*set, row);
    }
}

void SEASON3B::CCosmeticSetsWindow::RenderSetRow(const CosmeticSet& set, int row)
{
    const bool isHovered = IsRowHovered(row);
    if (set.Number == m_selectedSetNumber)
    {
        RenderRowHighlight(row, SelectedBackgroundColor);
    }
    else if (isHovered)
    {
        RenderRowHighlight(row, HoverBackgroundColor);
    }

    // The picture of the armor is drawn by Render3D, the texts go at its right: the name and below it the state.
    const bool isWorn = IsWorn(set);
    const auto textX = m_Pos.x + CONTENT_LEFT + ICON_WIDTH + ICON_GAP;
    const auto textWidth = CONTENT_WIDTH - ICON_WIDTH - ICON_GAP;
    const auto y = GetRowY(row) + (ROW_HEIGHT - 2 * TEXT_LINE_HEIGHT) / 2;

    UseTextColor(isHovered ? HoveredColor : GetNameColor(set, isWorn));
    RenderLine(textX, y, set.Name.c_str(), textWidth);

    UseTextColor(isWorn ? WornColor : LockedColor);
    RenderLine(textX, y + TEXT_LINE_HEIGHT, GetStateText(set, isWorn), textWidth);
}

void SEASON3B::CCosmeticSetsWindow::RenderRowHighlight(int row, unsigned int color)
{
    // RenderColor ignores the current color and draws white, so the color goes with the quad.
    RenderColorQuadARGB(static_cast<float>(m_Pos.x + CONTENT_LEFT), static_cast<float>(GetRowY(row)),
                        static_cast<float>(CONTENT_WIDTH), static_cast<float>(ROW_HEIGHT), color);
    EndRenderColor();
}

void SEASON3B::CCosmeticSetsWindow::RenderHint()
{
    UseTextColor(HintColor);
    for (size_t line = 0; line < m_hintLines.size(); ++line)
    {
        const auto y = m_Pos.y + HINT_TOP + static_cast<int>(line) * TEXT_LINE_HEIGHT;
        RenderLine(m_Pos.x + CONTENT_LEFT, y, m_hintLines[line].c_str(), CONTENT_WIDTH, RT3_SORT_CENTER);
    }
}

void SEASON3B::CCosmeticSetsWindow::RenderButtons()
{
    if (CanWearSelected())
    {
        m_BtnWear.SetFont(g_hFont);
        m_BtnWear.Render();
    }

    if (CosmeticSets().GetWornSet() != nullptr)
    {
        m_BtnTakeOff.SetFont(g_hFont);
        m_BtnTakeOff.Render();
    }
}

void SEASON3B::CCosmeticSetsWindow::LoadImages()
{
    // The ids are shared with the other windows, but every window loads what it draws.
    LoadBitmap(L"Interface/newui_msgbox_back.jpg", IMAGE_COSMETICSETS_BACK, GL_LINEAR);
    LoadBitmap(L"Interface/newui_item_back04.tga", IMAGE_COSMETICSETS_TOP, GL_LINEAR);
    LoadBitmap(L"Interface/newui_item_back02-L.tga", IMAGE_COSMETICSETS_LEFT, GL_LINEAR);
    LoadBitmap(L"Interface/newui_item_back02-R.tga", IMAGE_COSMETICSETS_RIGHT, GL_LINEAR);
    LoadBitmap(L"Interface/newui_item_back03.tga", IMAGE_COSMETICSETS_BOTTOM, GL_LINEAR);
    LoadBitmap(L"Interface/newui_exit_00.tga", IMAGE_COSMETICSETS_BTN_EXIT, GL_LINEAR);
    LoadBitmap(L"Interface/newui_btn_empty_small.tga", IMAGE_COSMETICSETS_BTN, GL_LINEAR);
}

void SEASON3B::CCosmeticSetsWindow::UnloadImages()
{
    DeleteBitmap(IMAGE_COSMETICSETS_BACK);
    DeleteBitmap(IMAGE_COSMETICSETS_TOP);
    DeleteBitmap(IMAGE_COSMETICSETS_LEFT);
    DeleteBitmap(IMAGE_COSMETICSETS_RIGHT);
    DeleteBitmap(IMAGE_COSMETICSETS_BOTTOM);
    DeleteBitmap(IMAGE_COSMETICSETS_BTN_EXIT);
    DeleteBitmap(IMAGE_COSMETICSETS_BTN);
}
