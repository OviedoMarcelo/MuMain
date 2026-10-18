// CosmeticsWindow.cpp: implementation of the CCosmeticsWindow class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "I18N/All.h"

#include "UI/NewUI/Inventory/CosmeticsWindow.h"

#include "Audio/DSPlaySound.h"
#include "Core/Text/TextLineWrap.h"
#include "Engine/Object/ZzzInventory.h"
#include "GameLogic/Commands/ChatCommandCatalog.h"
#include "UI/NewUI/NewUISystem.h"

#include <algorithm>
#include <string>

using namespace SEASON3B;
using GameLogic::Cosmetics::CosmeticPiece;
using GameLogic::Cosmetics::CosmeticPieces;

namespace
{
// The chat command which puts on a piece, or takes one off with "off <number>".
constexpr const wchar_t* WearCommand = L"/cosmetico ";
constexpr const wchar_t* TakeOffArgument = L"off ";
constexpr WORD NoPiece = 0;

// The item group of the helms; the groups of the other slots follow it.
constexpr BYTE HelmGroup = 7;

constexpr const wchar_t* LeftArrow = L"<";
constexpr const wchar_t* RightArrow = L">";

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
// The band behind the slot, a bit lighter than the panel.
constexpr unsigned int SlotBackgroundColor = 0x40FFFFFFu;

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

const wchar_t* GetSlotName(int slot)
{
    switch (slot)
    {
    case 0:
        return I18N::Game::CosmeticSlotHelm;
    case 1:
        return I18N::Game::CosmeticSlotArmor;
    case 2:
        return I18N::Game::CosmeticSlotPants;
    case 3:
        return I18N::Game::CosmeticSlotGloves;
    default:
        return I18N::Game::CosmeticSlotBoots;
    }
}

const wchar_t* GetStateText(const CosmeticPiece& piece)
{
    if (piece.IsWorn)
    {
        return I18N::Game::CosmeticsWorn;
    }

    if (!piece.IsUnlocked)
    {
        return I18N::Game::CosmeticsLocked;
    }

    return piece.CanWear ? L"" : I18N::Game::CosmeticsOtherClass;
}

const TextColor& GetNameColor(const CosmeticPiece& piece)
{
    if (piece.IsWorn)
    {
        return WornColor;
    }

    return piece.IsUnlocked && piece.CanWear ? NormalColor : LockedColor;
}
} // namespace

SEASON3B::CCosmeticsWindow::CCosmeticsWindow()
{
    m_pNewUIMng = nullptr;
    m_pNewUI3DRenderMng = nullptr;
    m_Pos.x = 0;
    m_Pos.y = 0;
    m_slot = 0;
    m_scrollOffset = 0;
    m_selectedPieceNumber = NoPiece;
    m_slotPiecesRevision = 0;
}

SEASON3B::CCosmeticsWindow::~CCosmeticsWindow()
{
    Release();
}

bool SEASON3B::CCosmeticsWindow::Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng, int x, int y)
{
    if (pNewUIMng == nullptr || pNewUI3DRenderMng == nullptr)
    {
        return false;
    }

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_COSMETICS, this);

    m_pNewUI3DRenderMng = pNewUI3DRenderMng;
    m_pNewUI3DRenderMng->Add3DRenderObj(this, INVENTORY_CAMERA_Z_ORDER);

    LoadImages();
    SetPos(x, y);
    InitButtons();
    Show(false);

    return true;
}

void SEASON3B::CCosmeticsWindow::Release()
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

void SEASON3B::CCosmeticsWindow::SetPos(int x, int y)
{
    m_Pos.x = x;
    m_Pos.y = y;

    m_BtnExit.ChangeButtonInfo(m_Pos.x + EXIT_BUTTON_X, m_Pos.y + EXIT_BUTTON_Y, EXIT_BUTTON_WIDTH, EXIT_BUTTON_HEIGHT);
    m_BtnWear.ChangeButtonInfo(m_Pos.x + CONTENT_LEFT, m_Pos.y + BUTTON_ROW_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
    m_BtnTakeOff.ChangeButtonInfo(m_Pos.x + WINDOW_WIDTH - CONTENT_LEFT - BUTTON_WIDTH, m_Pos.y + BUTTON_ROW_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
}

void SEASON3B::CCosmeticsWindow::InitButtons()
{
    m_BtnExit.ChangeButtonImgState(true, IMAGE_COSMETICS_BTN_EXIT);
    m_BtnExit.ChangeToolTipText(&I18N::Game::Close388, true);

    m_BtnWear.ChangeButtonImgState(true, IMAGE_COSMETICS_BTN, true);
    m_BtnWear.ChangeText(&I18N::Game::CosmeticsWear);

    m_BtnTakeOff.ChangeButtonImgState(true, IMAGE_COSMETICS_BTN, true);
    m_BtnTakeOff.ChangeText(&I18N::Game::CosmeticsTakeOff);
}

float SEASON3B::CCosmeticsWindow::GetLayerDepth()
{
    return LayerDepth;
}

float SEASON3B::CCosmeticsWindow::GetKeyEventOrder()
{
    return 10.f;
}

bool SEASON3B::CCosmeticsWindow::IsVisible() const
{
    return CNewUIObj::IsVisible();
}

void SEASON3B::CCosmeticsWindow::OpenningProcess()
{
    WrapHint();
    ShowSlot(m_slot);
}

void SEASON3B::CCosmeticsWindow::ClosingProcess()
{
    m_hintLines.clear();
}

void SEASON3B::CCosmeticsWindow::WrapHint()
{
    g_pRenderText->SetFont(g_hFont);
    m_hintLines = WrapTextToWidth(I18N::Game::CosmeticsHint, CONTENT_WIDTH, MeasureInReferenceUnits);
    if (m_hintLines.size() > HINT_LINES)
    {
        m_hintLines.resize(HINT_LINES);
    }
}

BYTE SEASON3B::CCosmeticsWindow::GetShownItemGroup() const
{
    return static_cast<BYTE>(HelmGroup + m_slot);
}

void SEASON3B::CCosmeticsWindow::ShowSlot(int slot)
{
    m_slot = (slot % SLOT_COUNT + SLOT_COUNT) % SLOT_COUNT;
    m_scrollOffset = 0;
    RefreshSlotPieces();

    // The worn piece of the slot is picked, so that it can be taken off right away.
    const auto* worn = CosmeticPieces().GetWornPiece(GetShownItemGroup());
    m_selectedPieceNumber = worn != nullptr ? worn->Number : NoPiece;
}

void SEASON3B::CCosmeticsWindow::RefreshSlotPieces()
{
    const auto& pieces = CosmeticPieces().GetPieces();
    const auto group = GetShownItemGroup();
    m_slotPieces.clear();
    for (size_t i = 0; i < pieces.size(); ++i)
    {
        // Only the pieces the character has; the locked ones aren't shown.
        if (pieces[i].ItemGroup == group && pieces[i].IsUnlocked)
        {
            m_slotPieces.push_back(i);
        }
    }

    m_slotPiecesRevision = CosmeticPieces().GetRevision();
}

const CosmeticPiece* SEASON3B::CCosmeticsWindow::GetPieceAt(int row) const
{
    if (row < 0 || static_cast<size_t>(row) >= m_slotPieces.size())
    {
        return nullptr;
    }

    const auto& pieces = CosmeticPieces().GetPieces();
    const auto index = m_slotPieces[row];
    return index < pieces.size() ? &pieces[index] : nullptr;
}

const CosmeticPiece* SEASON3B::CCosmeticsWindow::GetSelectedPiece() const
{
    for (int row = 0; static_cast<size_t>(row) < m_slotPieces.size(); ++row)
    {
        const auto* piece = GetPieceAt(row);
        if (piece != nullptr && piece->Number == m_selectedPieceNumber)
        {
            return piece;
        }
    }

    return nullptr;
}

bool SEASON3B::CCosmeticsWindow::CanWearSelected() const
{
    const auto* piece = GetSelectedPiece();
    return piece != nullptr && piece->IsUnlocked && piece->CanWear && !piece->IsWorn;
}

int SEASON3B::CCosmeticsWindow::GetRowY(int row) const
{
    return m_Pos.y + LIST_TOP + row * ROW_HEIGHT;
}

bool SEASON3B::CCosmeticsWindow::IsRowHovered(int row) const
{
    return CheckMouseIn(m_Pos.x + CONTENT_LEFT, GetRowY(row), CONTENT_WIDTH, ROW_HEIGHT);
}

bool SEASON3B::CCosmeticsWindow::IsArrowHovered(bool isRight) const
{
    const auto x = isRight ? m_Pos.x + CONTENT_LEFT + CONTENT_WIDTH - ARROW_WIDTH : m_Pos.x + CONTENT_LEFT;
    return CheckMouseIn(x, m_Pos.y + CONTENT_TOP, ARROW_WIDTH, SLOT_ROW_HEIGHT);
}

void SEASON3B::CCosmeticsWindow::Scroll(int rows)
{
    const auto hiddenRows = static_cast<int>(m_slotPieces.size()) - VISIBLE_ROWS;
    if (hiddenRows <= 0)
    {
        m_scrollOffset = 0;
        return;
    }

    m_scrollOffset = std::clamp(m_scrollOffset + rows, 0, hiddenRows);
}

bool SEASON3B::CCosmeticsWindow::UpdateMouseEvent()
{
    if (g_pNewUISystem->HandleFrameCornerClose(m_Pos, SEASON3B::INTERFACE_COSMETICS))
    {
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    if (m_BtnExit.UpdateMouseEvent())
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_COSMETICS);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    if (UpdateSlotMouseEvent() || UpdateButtonsMouseEvent() || UpdateListMouseEvent())
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

bool SEASON3B::CCosmeticsWindow::UpdateSlotMouseEvent()
{
    if (!IsRelease(VK_LBUTTON))
    {
        return false;
    }

    for (const bool isRight : {false, true})
    {
        if (IsArrowHovered(isRight))
        {
            PlayBuffer(SOUND_CLICK01);
            ShowSlot(m_slot + (isRight ? 1 : -1));
            return true;
        }
    }

    return false;
}

bool SEASON3B::CCosmeticsWindow::UpdateButtonsMouseEvent()
{
    // The server answers with a new CosmeticPieces message, which updates the list and the armor.
    if (CanWearSelected() && m_BtnWear.UpdateMouseEvent())
    {
        PlayBuffer(SOUND_CLICK01);
        GameLogic::Commands::ChatCommandCatalog::Execute(std::wstring(WearCommand) + std::to_wstring(m_selectedPieceNumber));
        return true;
    }

    const auto* worn = CosmeticPieces().GetWornPiece(GetShownItemGroup());
    if (worn != nullptr && m_BtnTakeOff.UpdateMouseEvent())
    {
        PlayBuffer(SOUND_CLICK01);
        GameLogic::Commands::ChatCommandCatalog::Execute(std::wstring(WearCommand) + TakeOffArgument + std::to_wstring(worn->Number));
        return true;
    }

    return false;
}

bool SEASON3B::CCosmeticsWindow::UpdateListMouseEvent()
{
    for (int row = 0; row < VISIBLE_ROWS; ++row)
    {
        const auto* piece = GetPieceAt(m_scrollOffset + row);
        if (piece == nullptr)
        {
            break;
        }

        if (IsRowHovered(row) && IsRelease(VK_LBUTTON))
        {
            PlayBuffer(SOUND_CLICK01);
            m_selectedPieceNumber = piece->Number;
            return true;
        }
    }

    return false;
}

bool SEASON3B::CCosmeticsWindow::UpdateKeyEvent()
{
    if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_COSMETICS))
    {
        return true;
    }

    if (IsPress(VK_ESCAPE))
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_COSMETICS);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    if (IsPress(VK_LEFT) || IsPress(VK_RIGHT))
    {
        ShowSlot(m_slot + (IsPress(VK_RIGHT) ? 1 : -1));
        return false;
    }

    if (IsPress(VK_DOWN) || IsPress(VK_UP))
    {
        Scroll(IsPress(VK_DOWN) ? 1 : -1);
        return false;
    }

    return true;
}

bool SEASON3B::CCosmeticsWindow::Update()
{
    if (m_slotPiecesRevision != CosmeticPieces().GetRevision())
    {
        RefreshSlotPieces();
    }

    // The list may have become shorter, e.g. after the configuration changed.
    Scroll(0);
    return true;
}

bool SEASON3B::CCosmeticsWindow::Render()
{
    EnableAlphaTest();
    glColor4f(1.f, 1.f, 1.f, 1.f);

    g_pRenderText->SetFont(g_hFont);
    UseTextColor(NormalColor);

    RenderBaseWindow();
    RenderTitle();
    RenderPanel();
    RenderSlotRow();
    RenderList();
    RenderHint();
    RenderButtons();

    m_BtnExit.Render();
    DisableAlphaBlend();
    return true;
}

void SEASON3B::CCosmeticsWindow::Render3D()
{
    for (int row = 0; row < VISIBLE_ROWS; ++row)
    {
        const auto* piece = GetPieceAt(m_scrollOffset + row);
        if (piece == nullptr)
        {
            break;
        }

        const int itemType = static_cast<int>(ITEMINDEX(piece->ItemGroup, piece->ItemNumber));
        const auto x = static_cast<float>(m_Pos.x + CONTENT_LEFT);
        const auto y = static_cast<float>(GetRowY(row) + (ROW_HEIGHT - ICON_HEIGHT) / 2);
        ::RenderItem3D(x, y, static_cast<float>(ICON_WIDTH), static_cast<float>(ICON_HEIGHT), itemType, piece->Level,
                       piece->IsExcellent ? 1 : 0, 0, false);
    }
}

void SEASON3B::CCosmeticsWindow::RenderBaseWindow()
{
    const auto x = static_cast<float>(m_Pos.x);
    const auto y = static_cast<float>(m_Pos.y);
    const auto middleHeight = static_cast<float>(WindowHeight - FRAME_TOP_HEIGHT - FRAME_BOTTOM_HEIGHT);

    RenderImage(IMAGE_COSMETICS_BACK, x, y, float(WINDOW_WIDTH), float(WindowHeight));
    RenderImage(IMAGE_COSMETICS_TOP, x, y, float(WINDOW_WIDTH), float(FRAME_TOP_HEIGHT));

    // Like in the quests window, the side pieces are stretched, because the window is taller than they are.
    RenderImageStretch(IMAGE_COSMETICS_LEFT, x, y + float(FRAME_TOP_HEIGHT), float(FRAME_SIDE_WIDTH), middleHeight,
                       0.f, 0.f, float(FRAME_SIDE_WIDTH), float(FRAME_SIDE_TEXTURE_HEIGHT));
    RenderImageStretch(IMAGE_COSMETICS_RIGHT, x + float(WINDOW_WIDTH - FRAME_SIDE_WIDTH), y + float(FRAME_TOP_HEIGHT),
                       float(FRAME_SIDE_WIDTH), middleHeight, 0.f, 0.f, float(FRAME_SIDE_WIDTH),
                       float(FRAME_SIDE_TEXTURE_HEIGHT));

    RenderImage(IMAGE_COSMETICS_BOTTOM, x, y + float(WindowHeight - FRAME_BOTTOM_HEIGHT), float(WINDOW_WIDTH),
                float(FRAME_BOTTOM_HEIGHT));
}

void SEASON3B::CCosmeticsWindow::RenderTitle()
{
    g_pRenderText->SetFont(g_hFontBold);
    UseTextColor(TitleColor);
    RenderLine(m_Pos.x, m_Pos.y + TITLE_Y, I18N::Game::CosmeticsTitle, WINDOW_WIDTH, RT3_SORT_CENTER);
    g_pRenderText->SetFont(g_hFont);
}

void SEASON3B::CCosmeticsWindow::RenderPanel()
{
    RenderColorQuadARGB(static_cast<float>(m_Pos.x + PANEL_LEFT), static_cast<float>(m_Pos.y + PANEL_TOP),
                        static_cast<float>(PANEL_WIDTH), static_cast<float>(PANEL_HEIGHT), PanelBackdropColor);
    EndRenderColor();
}

void SEASON3B::CCosmeticsWindow::RenderSlotRow()
{
    const auto y = m_Pos.y + CONTENT_TOP;
    RenderColorQuadARGB(static_cast<float>(m_Pos.x + CONTENT_LEFT), static_cast<float>(y), static_cast<float>(CONTENT_WIDTH),
                        static_cast<float>(SLOT_ROW_HEIGHT), SlotBackgroundColor);
    EndRenderColor();

    const auto textY = y + (SLOT_ROW_HEIGHT - TEXT_LINE_HEIGHT) / 2;
    g_pRenderText->SetFont(g_hFontBold);
    UseTextColor(IsArrowHovered(false) ? HoveredColor : TitleColor);
    RenderLine(m_Pos.x + CONTENT_LEFT, textY, LeftArrow, ARROW_WIDTH, RT3_SORT_CENTER);
    UseTextColor(IsArrowHovered(true) ? HoveredColor : TitleColor);
    RenderLine(m_Pos.x + CONTENT_LEFT + CONTENT_WIDTH - ARROW_WIDTH, textY, RightArrow, ARROW_WIDTH, RT3_SORT_CENTER);
    UseTextColor(TitleColor);
    RenderLine(m_Pos.x + CONTENT_LEFT + ARROW_WIDTH, textY, GetSlotName(m_slot), CONTENT_WIDTH - 2 * ARROW_WIDTH, RT3_SORT_CENTER);
    g_pRenderText->SetFont(g_hFont);
}

void SEASON3B::CCosmeticsWindow::RenderList()
{
    if (m_slotPieces.empty())
    {
        UseTextColor(NormalColor);
        RenderLine(m_Pos.x + CONTENT_LEFT, m_Pos.y + LIST_TOP, I18N::Game::CosmeticsNone, CONTENT_WIDTH);
        return;
    }

    for (int row = 0; row < VISIBLE_ROWS; ++row)
    {
        const auto* piece = GetPieceAt(m_scrollOffset + row);
        if (piece == nullptr)
        {
            break;
        }

        RenderPieceRow(*piece, row);
    }
}

void SEASON3B::CCosmeticsWindow::RenderPieceRow(const CosmeticPiece& piece, int row)
{
    const bool isHovered = IsRowHovered(row);
    if (piece.Number == m_selectedPieceNumber)
    {
        RenderRowHighlight(row, SelectedBackgroundColor);
    }
    else if (isHovered)
    {
        RenderRowHighlight(row, HoverBackgroundColor);
    }

    // The picture of the piece is drawn by Render3D, the texts go at its right: the name and below it the state.
    const auto textX = m_Pos.x + CONTENT_LEFT + ICON_WIDTH + ICON_GAP;
    const auto textWidth = CONTENT_WIDTH - ICON_WIDTH - ICON_GAP;
    const auto y = GetRowY(row) + (ROW_HEIGHT - 2 * TEXT_LINE_HEIGHT) / 2;

    UseTextColor(isHovered ? HoveredColor : GetNameColor(piece));
    RenderLine(textX, y, piece.Name.c_str(), textWidth);

    UseTextColor(piece.IsWorn ? WornColor : LockedColor);
    RenderLine(textX, y + TEXT_LINE_HEIGHT, GetStateText(piece), textWidth);
}

void SEASON3B::CCosmeticsWindow::RenderRowHighlight(int row, unsigned int color)
{
    // RenderColor ignores the current color and draws white, so the color goes with the quad.
    RenderColorQuadARGB(static_cast<float>(m_Pos.x + CONTENT_LEFT), static_cast<float>(GetRowY(row)),
                        static_cast<float>(CONTENT_WIDTH), static_cast<float>(ROW_HEIGHT), color);
    EndRenderColor();
}

void SEASON3B::CCosmeticsWindow::RenderHint()
{
    UseTextColor(HintColor);
    for (size_t line = 0; line < m_hintLines.size(); ++line)
    {
        const auto y = m_Pos.y + HINT_TOP + static_cast<int>(line) * TEXT_LINE_HEIGHT;
        RenderLine(m_Pos.x + CONTENT_LEFT, y, m_hintLines[line].c_str(), CONTENT_WIDTH, RT3_SORT_CENTER);
    }
}

void SEASON3B::CCosmeticsWindow::RenderButtons()
{
    if (CanWearSelected())
    {
        m_BtnWear.SetFont(g_hFont);
        m_BtnWear.Render();
    }

    if (CosmeticPieces().GetWornPiece(GetShownItemGroup()) != nullptr)
    {
        m_BtnTakeOff.SetFont(g_hFont);
        m_BtnTakeOff.Render();
    }
}

void SEASON3B::CCosmeticsWindow::LoadImages()
{
    // The ids are shared with the other windows, but every window loads what it draws.
    LoadBitmap(L"Interface/newui_msgbox_back.jpg", IMAGE_COSMETICS_BACK, GL_LINEAR);
    LoadBitmap(L"Interface/newui_item_back04.tga", IMAGE_COSMETICS_TOP, GL_LINEAR);
    LoadBitmap(L"Interface/newui_item_back02-L.tga", IMAGE_COSMETICS_LEFT, GL_LINEAR);
    LoadBitmap(L"Interface/newui_item_back02-R.tga", IMAGE_COSMETICS_RIGHT, GL_LINEAR);
    LoadBitmap(L"Interface/newui_item_back03.tga", IMAGE_COSMETICS_BOTTOM, GL_LINEAR);
    LoadBitmap(L"Interface/newui_exit_00.tga", IMAGE_COSMETICS_BTN_EXIT, GL_LINEAR);
    LoadBitmap(L"Interface/newui_btn_empty_small.tga", IMAGE_COSMETICS_BTN, GL_LINEAR);
}

void SEASON3B::CCosmeticsWindow::UnloadImages()
{
    DeleteBitmap(IMAGE_COSMETICS_BACK);
    DeleteBitmap(IMAGE_COSMETICS_TOP);
    DeleteBitmap(IMAGE_COSMETICS_LEFT);
    DeleteBitmap(IMAGE_COSMETICS_RIGHT);
    DeleteBitmap(IMAGE_COSMETICS_BOTTOM);
    DeleteBitmap(IMAGE_COSMETICS_BTN_EXIT);
    DeleteBitmap(IMAGE_COSMETICS_BTN);
}
