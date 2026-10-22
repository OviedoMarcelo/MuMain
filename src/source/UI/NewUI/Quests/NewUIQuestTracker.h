// NewUIQuestTracker.h: interface for the CNewUIQuestTracker class.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"
#include "UI/Quests/QuestTracker.h"

#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace SEASON3B
{
// Shows the quests in progress at the top right of the screen, in a frame with
// the title "Quests". An open quest shows its current step, its progress and
// its rewards; a folded one only its name and progress. A click on a quest opens
// or folds it in its place; at first only the first quest is open. The arrow in
// the title folds the whole tracker. The tracker moves to the left of the party
// list when there is a party, and stays behind the windows which open at the
// right, like the inventory.
class CNewUIQuestTracker : public CNewUIObj
{
    enum eLAYOUT
    {
        TRACKER_WIDTH = 176,
        TRACKER_TOP = 30,
        // The space between the tracker and the party list.
        TRACKER_PARTY_GAP = 4,
        TRACKER_MAX_QUESTS = 4,
        // The space between the frame and the quests in it.
        TRACKER_MARGIN = 4,
        HEADER_HEIGHT = 22,
        FOLD_BUTTON_SIZE = 14,
        // The open quest: the top of its lines within it.
        CARD_PADDING = 6,
        CARD_TEXT_INDENT = 20,
        CARD_NAME_Y = 6,
        CARD_OBJECTIVE_Y = 21,
        CARD_PROGRESS_Y = 35,
        CARD_BAR_Y = 47,
        CARD_BAR_HEIGHT = 6,
        CARD_SECTION_GAP = 6,
        CARD_FOOTER_HEIGHT = 18,
        BADGE_PADDING = 4,
        BADGE_HEIGHT = 11,
        BADGE_GAP = 5,
        // The folded quests below the open one.
        ROW_HEIGHT = 17,
        ROW_GAP = 3,
        ROW_COUNT_WIDTH = 30,
        ROW_ARROW_WIDTH = 16,
    };

    // An open quest, with its texts cut to fit.
    struct OpenQuest
    {
        std::wstring Id;
        // The top of the quest, relative to the top of the tracker.
        int Top = 0;
        std::wstring Name;
        std::wstring Badge;
        int BadgeWidth = 0;
        std::wstring Objective;
        std::wstring Count;
        float Ratio = 0.0f;
        bool HasProgress = false;
        std::wstring Footer;
        bool IsRewardPending = false;
        // The top of the separator above the footer, relative to the top of the quest.
        int FooterTop = 0;
        int Height = 0;
    };

    // A folded quest, with its texts cut to fit.
    struct FoldedQuest
    {
        std::wstring Id;
        std::wstring Name;
        std::wstring Count;
        bool IsRewardPending = false;
        // The top of the row, relative to the top of the tracker.
        int Top = 0;
    };

public:
    CNewUIQuestTracker();
    ~CNewUIQuestTracker() override;

    bool Create(CNewUIManager* pNewUIMng);
    void Release();

    bool UpdateMouseEvent() override;
    bool UpdateKeyEvent() override;
    bool Update() override;
    bool Render() override;
    float GetLayerDepth() override;

private:
    // Lays out the quests again when they changed. It needs the font of the
    // text renderer to measure the texts, so it's done while rendering.
    void Layout();
    // Returns the height of the quest.
    int LayoutOpenQuest(const UI::Quests::TrackedQuest& quest, int top);
    void LayoutFoldedQuest(const UI::Quests::TrackedQuest& quest, int top);
    void Invalidate();
    void ToggleQuest(const std::wstring& id);

    int GetLeft() const;
    int GetContentWidth() const;
    bool IsMouseOverFoldButton() const;
    bool IsMouseOverQuest(int top, int height) const;
    // The quest under the mouse; nullptr when there is none.
    const std::wstring* FindQuestUnderMouse() const;

    void RenderFrame(int left);
    void RenderHeader(int left);
    void RenderOpenQuest(int left, const OpenQuest& quest);
    void RenderOpenQuestProgress(int x, int top, int width, const OpenQuest& quest);
    void RenderOpenQuestFooter(int x, int top, int width, const OpenQuest& quest);
    void RenderFoldedQuest(int left, const FoldedQuest& quest);

    CNewUIManager* m_pNewUIMng = nullptr;
    std::wstring m_title;
    bool m_hasQuests = false;
    std::vector<OpenQuest> m_openQuests;
    std::vector<FoldedQuest> m_foldedQuests;
    int m_height = 0;
    // The quests which are open. Until the player opens or folds one, the first quest is open.
    std::set<std::wstring> m_openQuestIds;
    bool m_hasChosenOpenQuests = false;
    bool m_isFolded = false;
    uint32_t m_layoutRevision = 0;
    bool m_isLaidOut = false;
};
} // namespace SEASON3B
