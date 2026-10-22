// CosmeticsWindow.h: interface for the CCosmeticsWindow class.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "UI/NewUI/Dialogs/NewUIMessageBox.h"
#include "UI/NewUI/NewUI3DRenderMng.h"
#include "UI/NewUI/UILayoutPolicy.h"
#include "UI/NewUI/Widgets/NewUIButton.h"
#include "UI/Scaling/UITransform.h"
#include "GameLogic/Cosmetics/CosmeticPieceCatalog.h"

#include <string>
#include <vector>

namespace SEASON3B
{
// Shows the cosmetic pieces which the server told us about, at the left of the
// inventory, one slot at a time (the armor, the wings and the pet). A piece can
// be picked and worn over the item of its slot: it only changes how it looks,
// for the own character and for everyone else.
//
// It uses the frame and the size of the quests window, so that the windows
// look alike when they are docked at the same place.
class CCosmeticsWindow : public CNewUIObj, public INewUI3DRenderObj
{
    enum eIMAGE_LIST
    {
        IMAGE_COSMETICS_BACK = CNewUIMessageBoxMng::IMAGE_MSGBOX_BACK,
        IMAGE_COSMETICS_TOP = CNewUIMyInventory::IMAGE_INVENTORY_BACK_TOP2,
        IMAGE_COSMETICS_LEFT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_LEFT,
        IMAGE_COSMETICS_RIGHT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_RIGHT,
        IMAGE_COSMETICS_BOTTOM = CNewUIMyInventory::IMAGE_INVENTORY_BACK_BOTTOM,
        IMAGE_COSMETICS_BTN_EXIT = CNewUIMyInventory::IMAGE_INVENTORY_EXIT_BTN,
        IMAGE_COSMETICS_BTN = CNewUIMessageBoxMng::IMAGE_MSGBOX_BTN_EMPTY_SMALL,
    };

    enum eWINDOW_SIZE
    {
        WINDOW_WIDTH = 190,
        FRAME_TOP_HEIGHT = 64,
        FRAME_SIDE_WIDTH = 21,
        FRAME_BOTTOM_HEIGHT = 45,
        // The side pieces are only this tall, see the quests window.
        FRAME_SIDE_TEXTURE_HEIGHT = 320,
    };

    enum eLAYOUT
    {
        TITLE_Y = 13,
        PANEL_LEFT = 15,
        PANEL_WIDTH = WINDOW_WIDTH - 2 * PANEL_LEFT,
        PANEL_TOP = 50,
        PANEL_PADDING = 7,
        CONTENT_LEFT = PANEL_LEFT + PANEL_PADDING,
        CONTENT_WIDTH = WINDOW_WIDTH - 2 * CONTENT_LEFT,
        CONTENT_TOP = PANEL_TOP + PANEL_PADDING,
        // The row which tells the slot, with an arrow at each side to change it.
        SLOT_ROW_HEIGHT = 16,
        ARROW_WIDTH = 18,
        LIST_TOP = CONTENT_TOP + SLOT_ROW_HEIGHT + PANEL_PADDING,
        // A row is as tall as the picture of the piece in it.
        ROW_HEIGHT = 30,
        ICON_WIDTH = 24,
        ICON_HEIGHT = 28,
        ICON_GAP = 4,
        TEXT_LINE_HEIGHT = 13,
        BUTTON_WIDTH = 64,
        BUTTON_HEIGHT = 29,
        BUTTON_ROW_Y = 358,
        EXIT_BUTTON_X = 13,
        EXIT_BUTTON_Y = 392,
        EXIT_BUTTON_WIDTH = 36,
        EXIT_BUTTON_HEIGHT = 29,
        // The hint takes up to two lines above the buttons.
        HINT_LINES = 2,
        HINT_TOP = BUTTON_ROW_Y - (HINT_LINES + 1) * TEXT_LINE_HEIGHT,
        VISIBLE_ROWS = (HINT_TOP - PANEL_PADDING - LIST_TOP) / ROW_HEIGHT,
        PANEL_HEIGHT = LIST_TOP - PANEL_TOP + VISIBLE_ROWS * ROW_HEIGHT + PANEL_PADDING,
        SLOT_COUNT = 7,
    };

public:
    static constexpr float LayerDepth = UI::Layout::ForegroundPanelLayerDepth;
    static constexpr int WindowHeight = UI::Scaling::DockLogicalBottom;
    static constexpr int WindowWidth = WINDOW_WIDTH;

    CCosmeticsWindow();
    ~CCosmeticsWindow() override;

    bool Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng, int x, int y);
    void Release();

    void SetPos(int x, int y);

    bool UpdateMouseEvent() override;
    bool UpdateKeyEvent() override;
    bool Update() override;
    bool Render() override;
    void Render3D() override;
    bool IsVisible() const override;

    float GetLayerDepth() override;
    float GetKeyEventOrder() override;

    void OpenningProcess();
    void ClosingProcess();

private:
    // The pieces of the shown slot are looked up again only when the slot or the pieces changed.
    void RefreshSlotPieces();
    void ShowSlot(int slot);
    BYTE GetShownItemGroup() const;
    const GameLogic::Cosmetics::CosmeticPiece* GetPieceAt(int row) const;
    const GameLogic::Cosmetics::CosmeticPiece* GetSelectedPiece() const;
    bool CanWearSelected() const;
    int GetRowY(int row) const;
    bool IsRowHovered(int row) const;
    bool IsArrowHovered(bool isRight) const;
    void Scroll(int rows);
    void WrapHint();

    void InitButtons();
    void LoadImages();
    void UnloadImages();

    bool UpdateSlotMouseEvent();
    bool UpdateListMouseEvent();
    bool UpdateButtonsMouseEvent();

    void RenderBaseWindow();
    void RenderTitle();
    void RenderPanel();
    void RenderSlotRow();
    void RenderList();
    void RenderPieceRow(const GameLogic::Cosmetics::CosmeticPiece& piece, int row);
    void RenderRowHighlight(int row, unsigned int color);
    void RenderHint();
    void RenderButtons();

private:
    CNewUIManager* m_pNewUIMng;
    CNewUI3DRenderMng* m_pNewUI3DRenderMng;
    POINT m_Pos;

    // The shown slot: 0 for the helm to 4 for the boots, 5 for the wings and 6 for the pet.
    int m_slot;
    int m_scrollOffset;
    // The piece is remembered by its number, because the list is replaced with every message.
    WORD m_selectedPieceNumber;
    // The indexes of the pieces of the shown slot in the catalog.
    std::vector<size_t> m_slotPieces;
    // The revision of the catalog which m_slotPieces was built from.
    uint32_t m_slotPiecesRevision;
    std::vector<std::wstring> m_hintLines;

    CNewUIButton m_BtnExit;
    CNewUIButton m_BtnWear;
    CNewUIButton m_BtnTakeOff;
};
} // namespace SEASON3B
