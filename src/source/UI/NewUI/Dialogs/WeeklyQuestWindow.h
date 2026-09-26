// WeeklyQuestWindow.h: interface for the CWeeklyQuestWindow class.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "UI/NewUI/Dialogs/NewUIMessageBox.h"
#include "UI/NewUI/UILayoutPolicy.h"
#include "UI/NewUI/Widgets/NewUIButton.h"
#include "UI/Scaling/UITransform.h"
#include "GameLogic/Quests/WeeklyQuestCatalog.h"

#include <string>
#include <vector>

namespace SEASON3B
{
// Shows the weekly quests which the server told us about, with the progress
// of the character. The first page lists the quests, a click on one shows
// its description and rewards.
//
// It uses the frame and the size of the chat commands window, so that both
// look alike when they are docked at the same place.
class CWeeklyQuestWindow : public CNewUIObj
{
    enum eIMAGE_LIST
    {
        IMAGE_WEEKLYQUEST_BACK = CNewUIMessageBoxMng::IMAGE_MSGBOX_BACK,
        // The top with a plate behind the title, like the pet window.
        IMAGE_WEEKLYQUEST_TOP = CNewUIMyInventory::IMAGE_INVENTORY_BACK_TOP2,
        IMAGE_WEEKLYQUEST_LEFT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_LEFT,
        IMAGE_WEEKLYQUEST_RIGHT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_RIGHT,
        IMAGE_WEEKLYQUEST_BOTTOM = CNewUIMyInventory::IMAGE_INVENTORY_BACK_BOTTOM,
        IMAGE_WEEKLYQUEST_BTN_EXIT = CNewUIMyInventory::IMAGE_INVENTORY_EXIT_BTN,
        IMAGE_WEEKLYQUEST_BTN = CNewUIMessageBoxMng::IMAGE_MSGBOX_BTN_EMPTY_SMALL,
    };

    enum eWINDOW_SIZE
    {
        WINDOW_WIDTH = 190,
        FRAME_TOP_HEIGHT = 64,
        FRAME_SIDE_WIDTH = 21,
        FRAME_BOTTOM_HEIGHT = 45,
        // The side pieces are only this tall, see the chat commands window.
        FRAME_SIDE_TEXTURE_HEIGHT = 320,
    };

    enum eLAYOUT
    {
        TITLE_Y = 13,
        // The content sits on a dark panel, like the boxes of the pet window.
        PANEL_LEFT = 15,
        PANEL_WIDTH = WINDOW_WIDTH - 2 * PANEL_LEFT,
        PANEL_TOP = 50,
        PANEL_PADDING = 7,
        CONTENT_LEFT = PANEL_LEFT + PANEL_PADDING,
        CONTENT_WIDTH = WINDOW_WIDTH - 2 * CONTENT_LEFT,
        CONTENT_TOP = PANEL_TOP + PANEL_PADDING,
        ROW_HEIGHT = 15,
        // The width which is reserved for the progress at the right of a quest.
        PROGRESS_WIDTH = 40,
        // The width of the arrow which tells that a quest can be clicked.
        ARROW_WIDTH = 10,
        BUTTON_WIDTH = 64,
        BUTTON_HEIGHT = 29,
        BUTTON_ROW_Y = 358,
        EXIT_BUTTON_X = 13,
        EXIT_BUTTON_Y = 392,
        EXIT_BUTTON_WIDTH = 36,
        EXIT_BUTTON_HEIGHT = 29,
        // The time until the reset is shown in the row above the buttons.
        RESET_ROW_Y = BUTTON_ROW_Y - ROW_HEIGHT,
        // The hint which tells how to see the details takes up to two lines above it.
        HINT_LINES = 2,
        HINT_TOP = RESET_ROW_Y - (HINT_LINES + 1) * ROW_HEIGHT,
        VISIBLE_ROWS = (HINT_TOP - PANEL_PADDING - CONTENT_TOP) / ROW_HEIGHT,
        LIST_PANEL_HEIGHT = VISIBLE_ROWS * ROW_HEIGHT + 2 * PANEL_PADDING,
        // The details page has no hint and no reset time, so it reaches down to its button.
        DETAIL_VISIBLE_ROWS = (BUTTON_ROW_Y - 2 * PANEL_PADDING - CONTENT_TOP) / ROW_HEIGHT,
        DETAIL_PANEL_HEIGHT = DETAIL_VISIBLE_ROWS * ROW_HEIGHT + 2 * PANEL_PADDING,
    };

    enum ePAGE
    {
        PAGE_LIST,
        PAGE_DETAILS,
    };

public:
    // How a line of the details page is drawn. It follows the quest window of
    // the original client: the name as a cyan heading, the description below
    // it, and yellow headings for the progress and the rewards.
    enum eDETAIL_STYLE
    {
        STYLE_TITLE,
        STYLE_DESCRIPTION,
        STYLE_HEADING,
        STYLE_VALUE,
        STYLE_COMPLETED,
        STYLE_PENDING,
    };

    static constexpr float LayerDepth = UI::Layout::ForegroundPanelLayerDepth;
    static constexpr int WindowHeight = UI::Scaling::DockLogicalBottom;

    CWeeklyQuestWindow();
    ~CWeeklyQuestWindow() override;

    bool Create(CNewUIManager* pNewUIMng, int x, int y);
    void Release();

    void SetPos(int x, int y);

    bool UpdateMouseEvent() override;
    bool UpdateKeyEvent() override;
    bool Update() override;
    bool Render() override;

    float GetLayerDepth() override;
    float GetKeyEventOrder() override;

    void OpenningProcess();
    void ClosingProcess();

private:
    const GameLogic::Quests::WeeklyQuest* GetQuestAt(int row) const;
    const GameLogic::Quests::WeeklyQuest* GetSelectedQuest() const;

    void ShowPage(ePAGE page);
    void PickQuest(int row);
    // The texts are wrapped once when the page is entered, because measuring
    // them against the font is too much work for every frame.
    void WrapDetailsOfSelected();
    void AddDetailLines(const std::wstring& text, eDETAIL_STYLE style);
    void WrapHint();
    int GetScrollableRowCount() const;
    int GetVisibleRowCount() const;
    bool IsRowHovered(int y) const;

    void InitButtons();
    void LoadImages();
    void UnloadImages();

    bool UpdateListPageMouseEvent();
    void Scroll(int rows);

    void RenderBaseWindow();
    void RenderTitle();
    void RenderListPage();
    void RenderQuestRow(const GameLogic::Quests::WeeklyQuest& quest, int y);
    void RenderRowHighlight(int y);
    void RenderPanel(int height);
    void RenderHint();
    void RenderDetailsPage();
    void RenderTimeUntilReset();

private:
    CNewUIManager* m_pNewUIMng;
    POINT m_Pos;

    ePAGE m_page;
    int m_selectedRow;
    int m_scrollOffset;
    // The revision of the catalog which the details page shows.
    uint32_t m_shownRevision;

    // The lines of the details page: the name, the description, the progress and the rewards.
    struct DetailLine
    {
        std::wstring Text;
        eDETAIL_STYLE Style = STYLE_DESCRIPTION;
    };
    std::vector<DetailLine> m_detailLines;
    std::vector<std::wstring> m_hintLines;

    CNewUIButton m_BtnExit;
    CNewUIButton m_BtnBack;
};
} // namespace SEASON3B
