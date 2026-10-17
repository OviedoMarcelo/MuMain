// NewUIEventScheduleWindow.h: interface for the CNewUIEventScheduleWindow class.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "UI/NewUI/Dialogs/NewUIMessageBox.h"
#include "UI/NewUI/UILayoutPolicy.h"
#include "UI/NewUI/Widgets/NewUIButton.h"
#include "UI/Scaling/UITransform.h"

namespace SEASON3B
{
// Shows when the events start which the server runs by itself, e.g. Blood
// Castle or the invasions: the next start of each one and how long until
// then, or how long a running event lasts.
//
// It has the frame and the width of the chat command window.
class CNewUIEventScheduleWindow : public CNewUIObj
{
    enum eIMAGE_LIST
    {
        IMAGE_EVENTSCHEDULE_BACK = CNewUIMessageBoxMng::IMAGE_MSGBOX_BACK,
        IMAGE_EVENTSCHEDULE_TOP = CNewUIMyInventory::IMAGE_INVENTORY_BACK_TOP,
        IMAGE_EVENTSCHEDULE_LEFT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_LEFT,
        IMAGE_EVENTSCHEDULE_RIGHT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_RIGHT,
        IMAGE_EVENTSCHEDULE_BOTTOM = CNewUIMyInventory::IMAGE_INVENTORY_BACK_BOTTOM,
        IMAGE_EVENTSCHEDULE_BTN_EXIT = CNewUIMyInventory::IMAGE_INVENTORY_EXIT_BTN,
    };

    enum eWINDOW_SIZE
    {
        WINDOW_WIDTH = 190,
        FRAME_TOP_HEIGHT = 64,
        FRAME_SIDE_WIDTH = 21,
        FRAME_BOTTOM_HEIGHT = 45,
        // The side pieces are only this tall; they're stretched for the rest.
        FRAME_SIDE_TEXTURE_HEIGHT = 320,
    };

    enum eLAYOUT
    {
        TITLE_Y = 12,
        CONTENT_LEFT = 22,
        CONTENT_WIDTH = WINDOW_WIDTH - 2 * CONTENT_LEFT,
        CONTENT_TOP = 36,
        ROW_HEIGHT = 15,
        // An event takes its name, its state, and a gap.
        EVENT_HEIGHT = 2 * ROW_HEIGHT + 4,
        SERVER_TIME_Y = 366,
        EXIT_BUTTON_X = 13,
        EXIT_BUTTON_Y = 392,
        EXIT_BUTTON_WIDTH = 36,
        EXIT_BUTTON_HEIGHT = 29,
        // Where the events have to end so that they don't run into the time of the server.
        CONTENT_BOTTOM = SERVER_TIME_Y - 4,
        VISIBLE_EVENTS = (CONTENT_BOTTOM - CONTENT_TOP) / EVENT_HEIGHT,
    };

public:
    static constexpr float LayerDepth = UI::Layout::ForegroundPanelLayerDepth;
    static constexpr int WindowHeight = UI::Scaling::DockLogicalBottom;

    CNewUIEventScheduleWindow();
    ~CNewUIEventScheduleWindow() override;

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
    void LoadImages();
    void UnloadImages();

    void RenderBaseWindow();
    void RenderEvents(int serverSecondOfDay);
    void RenderServerTime(int serverSecondOfDay);
    int GetHiddenEventCount() const;

private:
    CNewUIManager* m_pNewUIMng;
    POINT m_Pos;
    int m_scrollOffset;

    CNewUIButton m_BtnExit;
};
} // namespace SEASON3B
