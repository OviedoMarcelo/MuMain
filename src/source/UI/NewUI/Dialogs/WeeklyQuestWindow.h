// WeeklyQuestWindow.h: interface for the CWeeklyQuestWindow class.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "UI/NewUI/Dialogs/NewUIMessageBox.h"
#include "UI/NewUI/UILayoutPolicy.h"
#include "UI/NewUI/Widgets/NewUIButton.h"
#include "UI/Scaling/UITransform.h"
#include "GameLogic/Quests/SeasonPassCatalog.h"
#include "GameLogic/Quests/WeeklyQuestCatalog.h"
#include "UI/Quests/QuestList.h"

#include <string>
#include <vector>

namespace SEASON3B
{
// Shows the quests which the server told us about, with the progress of the
// character. The first page lists the quests grouped by category (story,
// daily, weekly, class, zone), a click on one shows its description, the
// checklist of its steps and its rewards.
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
        // The season pass: the level, the experience and the rewards of each level.
        PAGE_SEASON,
    };

public:
    // How a line of the details page is drawn. It follows the quest window of
    // the original client: the name as a cyan heading, the description below
    // it, and yellow headings for the progress and the rewards.
    enum eDETAIL_STYLE
    {
        STYLE_TITLE,
        // The type of the quest and when it resets, below the name.
        STYLE_TYPE,
        STYLE_DESCRIPTION,
        STYLE_HEADING,
        STYLE_VALUE,
        STYLE_COMPLETED,
        STYLE_PENDING,
        // The steps of the checklist: done, the one to do now, and the ones after it.
        STYLE_STEP_DONE,
        STYLE_STEP_CURRENT,
        STYLE_STEP_LATER,
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
    const UI::Quests::QuestListRow* GetRowAt(int row) const;
    const GameLogic::Quests::WeeklyQuest* GetQuestAt(int row) const;
    const GameLogic::Quests::WeeklyQuest* GetSelectedQuest() const;
    // The rows are built again only when the quests changed, not every frame.
    void RefreshRows();

    void ShowPage(ePAGE page);
    void PickQuest(int row);
    // The texts are wrapped once when the page is entered, because measuring
    // them against the font is too much work for every frame.
    void WrapDetailsOfSelected();
    void AddProgressLines(const GameLogic::Quests::WeeklyQuest& quest);
    void AddStepLines(const GameLogic::Quests::WeeklyQuest& quest);
    void AddDetailLines(const std::wstring& text, eDETAIL_STYLE style);
    // The season pass page uses the lines of the details page, built from the season pass.
    void WrapSeasonPass();
    void AddSeasonSummaryLines();
    void AddSeasonLevelLines(const GameLogic::Quests::SeasonPassLevel& level);
    bool UpdateSeasonPageMouseEvent();
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
    void RenderHeadingRow(GameLogic::Quests::QuestCategory category, int y);
    void RenderRowHighlight(int y);
    void RenderPanel(int height);
    void RenderHint();
    void RenderDetailsPage();
    void RenderTimeUntilReset();

private:
    CNewUIManager* m_pNewUIMng;
    POINT m_Pos;

    ePAGE m_page;
    // The quest is remembered by its id, because its row moves when the list is built again.
    std::wstring m_selectedQuestId;
    int m_scrollOffset;
    // The revision of the catalog which the details page shows.
    uint32_t m_shownRevision;

    std::vector<UI::Quests::QuestListRow> m_rows;
    // The revision of the catalog which the rows were built from.
    uint32_t m_rowsRevision;
    // The quest whose reset is shown below the list; NoQuest when nothing resets.
    int m_resetQuestIndex;

    // The lines of the details page: the name, the description, the progress and the rewards.
    struct DetailLine
    {
        std::wstring Text;
        eDETAIL_STYLE Style = STYLE_DESCRIPTION;
    };
    std::vector<DetailLine> m_detailLines;
    std::vector<std::wstring> m_hintLines;

    // The revision of the season pass which the season page shows.
    uint32_t m_seasonRevision;

    CNewUIButton m_BtnExit;
    CNewUIButton m_BtnBack;
    // On the list: opens the season pass. On the season pass: hands out the reached rewards.
    CNewUIButton m_BtnSeason;
    CNewUIButton m_BtnClaim;
};
} // namespace SEASON3B
