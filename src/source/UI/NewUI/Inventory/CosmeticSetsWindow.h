// CosmeticSetsWindow.h: interface for the CCosmeticSetsWindow class.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "UI/NewUI/Dialogs/NewUIMessageBox.h"
#include "UI/NewUI/NewUI3DRenderMng.h"
#include "UI/NewUI/UILayoutPolicy.h"
#include "UI/NewUI/Widgets/NewUIButton.h"
#include "UI/Scaling/UITransform.h"
#include "GameLogic/Cosmetics/CosmeticSetCatalog.h"

#include <string>
#include <vector>

namespace SEASON3B
{
// Shows the cosmetic sets which the server told us about, at the left of
// the inventory. A set can be picked and worn over the armor: it only
// changes how the armor looks, for the own character and for everyone else.
//
// It uses the frame and the size of the quests window, so that the windows
// look alike when they are docked at the same place.
class CCosmeticSetsWindow : public CNewUIObj, public INewUI3DRenderObj
{
    enum eIMAGE_LIST
    {
        IMAGE_COSMETICSETS_BACK = CNewUIMessageBoxMng::IMAGE_MSGBOX_BACK,
        IMAGE_COSMETICSETS_TOP = CNewUIMyInventory::IMAGE_INVENTORY_BACK_TOP2,
        IMAGE_COSMETICSETS_LEFT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_LEFT,
        IMAGE_COSMETICSETS_RIGHT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_RIGHT,
        IMAGE_COSMETICSETS_BOTTOM = CNewUIMyInventory::IMAGE_INVENTORY_BACK_BOTTOM,
        IMAGE_COSMETICSETS_BTN_EXIT = CNewUIMyInventory::IMAGE_INVENTORY_EXIT_BTN,
        IMAGE_COSMETICSETS_BTN = CNewUIMessageBoxMng::IMAGE_MSGBOX_BTN_EMPTY_SMALL,
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
        // A row is as tall as the picture of the armor in it.
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
        VISIBLE_ROWS = (HINT_TOP - PANEL_PADDING - CONTENT_TOP) / ROW_HEIGHT,
        PANEL_HEIGHT = VISIBLE_ROWS * ROW_HEIGHT + 2 * PANEL_PADDING,
    };

public:
    static constexpr float LayerDepth = UI::Layout::ForegroundPanelLayerDepth;
    static constexpr int WindowHeight = UI::Scaling::DockLogicalBottom;
    static constexpr int WindowWidth = WINDOW_WIDTH;

    CCosmeticSetsWindow();
    ~CCosmeticSetsWindow() override;

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
    const GameLogic::Cosmetics::CosmeticSet* GetSetAt(int row) const;
    const GameLogic::Cosmetics::CosmeticSet* GetSelectedSet() const;
    bool CanWearSelected() const;
    int GetRowY(int row) const;
    bool IsRowHovered(int row) const;
    void Scroll(int rows);
    void WrapHint();

    void InitButtons();
    void LoadImages();
    void UnloadImages();

    bool UpdateListMouseEvent();
    bool UpdateButtonsMouseEvent();

    void RenderBaseWindow();
    void RenderTitle();
    void RenderPanel();
    void RenderList();
    void RenderSetRow(const GameLogic::Cosmetics::CosmeticSet& set, int row);
    void RenderRowHighlight(int row, unsigned int color);
    void RenderHint();
    void RenderButtons();

private:
    CNewUIManager* m_pNewUIMng;
    CNewUI3DRenderMng* m_pNewUI3DRenderMng;
    POINT m_Pos;

    int m_scrollOffset;
    // The set is remembered by its number, because the list is replaced with every message.
    WORD m_selectedSetNumber;
    std::vector<std::wstring> m_hintLines;

    CNewUIButton m_BtnExit;
    CNewUIButton m_BtnWear;
    CNewUIButton m_BtnTakeOff;
};
} // namespace SEASON3B
