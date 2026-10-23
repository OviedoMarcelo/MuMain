// NpcQuestDialogWindow.h: interface for the CNpcQuestDialogWindow class.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"
#include "UI/NewUI/Quests/RisingParticles.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace GameLogic::Quests
{
struct NpcQuestDialog;
}

namespace SEASON3B
{
// Shows the dialog of an NPC whose talk is a step of a quest (see
// GameLogic::Quests::NpcQuestDialogState): the portrait of the NPC with its
// name below it, the name of the quest, what the NPC says, and the buttons
// "accept" and "not now". A long text is shown in pages: until the last one,
// the first button continues reading instead of accepting. The cross and escape
// decline on any page. The dialog closes
// by itself when the character walks away from the NPC.
//
// It fades in while it rises into its place, and the border of "accept" and
// the diamond above the buttons glow slowly. Some NPCs have an effect over
// their portrait, e.g. golden motes or fire sparks, see NpcPortraitEffects.
//
// The portrait is Data/Interface/NpcPortraits/<number of the NPC>.OZJ; NPCs
// without one show the MU Cyber emblem instead.
class CNpcQuestDialogWindow : public CNewUIObj
{
    static constexpr int PANEL_WIDTH = 360;
    static constexpr int PADDING = 8;
    // The space between the bottom of the panel and the main frame of the HUD.
    static constexpr int BOTTOM_MARGIN = 24;
    static constexpr int PORTRAIT_SIZE = 96;
    static constexpr int NAME_PLATE_GAP = 4;
    static constexpr int NAME_PLATE_HEIGHT = 18;
    static constexpr int MINIMUM_HEIGHT = PADDING + PORTRAIT_SIZE + NAME_PLATE_GAP + NAME_PLATE_HEIGHT + PADDING;
    static constexpr int PORTRAIT_TEXT_GAP = 12;
    static constexpr int TEXT_LEFT = PADDING + PORTRAIT_SIZE + PORTRAIT_TEXT_GAP;
    static constexpr int CLOSE_BUTTON_SIZE = 12;
    static constexpr int TEXT_WIDTH = PANEL_WIDTH - TEXT_LEFT - CLOSE_BUTTON_SIZE - 2 * PADDING;
    static constexpr int QUEST_NAME_TOP = 10;
    static constexpr int QUEST_NAME_HEIGHT = 16;
    static constexpr int TEXT_TOP = QUEST_NAME_TOP + QUEST_NAME_HEIGHT;
    static constexpr int LINE_HEIGHT = 12;
    // The lines of a page.
    static constexpr int MAXIMUM_LINES = 8;
    static constexpr int SEPARATOR_GAP = 6;
    static constexpr int BUTTON_WIDTH = PANEL_WIDTH - TEXT_LEFT - PADDING;
    static constexpr int BUTTON_HEIGHT = 20;
    static constexpr int BUTTON_GAP = 4;
    static constexpr int BUTTON_ICON_WIDTH = 22;

    enum class eBUTTON
    {
        // Continues reading while there are more pages, then accepts.
        Accept,
        Decline,
    };

public:
    CNpcQuestDialogWindow();
    ~CNpcQuestDialogWindow() override;

    bool Create(CNewUIManager* pNewUIMng);
    void Release();

    bool UpdateMouseEvent() override;
    bool UpdateKeyEvent() override;
    bool Update() override;
    bool Render() override;
    float GetLayerDepth() override;
    float GetKeyEventOrder() override;

private:
    // Wraps the text of the dialog and loads its portrait. It needs the font
    // of the text renderer to measure the text, so it's done while rendering.
    void Layout(const GameLogic::Quests::NpcQuestDialog& dialog);
    void LoadPortrait(uint16_t npcNumber);
    void LoadPortraitParticles(uint16_t npcNumber);
    void Answer(bool accepted);
    void AcceptOrContinue();
    int GetPageCount() const;
    bool IsLastPage() const;
    // Takes the time since the dialog opened, for the opening and the effects.
    void UpdateAnimation();

    int GetLeft() const;
    int GetTop() const;
    int GetButtonTop(eBUTTON button) const;
    bool IsMouseOverButton(eBUTTON button) const;
    bool IsMouseOverClose() const;

    void RenderFrame(int left, int top);
    void RenderPortrait(int left, int top);
    void RenderPortraitEffect(float x, float y);
    void RenderTexts(int left, int top);
    void RenderSeparator(int left, int top);
    void RenderButton(int left, eBUTTON button);
    unsigned int GetButtonBorder(eBUTTON button, bool isHovered) const;
    std::wstring GetButtonText(eBUTTON button) const;
    void RenderCloseButton(int left, int top);

    CNewUIManager* m_pNewUIMng = nullptr;
    bool m_isLaidOut = false;
    bool m_hasDialog = false;
    uint32_t m_layoutRevision = 0;

    std::wstring m_npcName;
    std::wstring m_questName;
    std::vector<std::wstring> m_lines;
    // The page of the lines which is shown, from 0.
    int m_page = 0;
    // The top of the separator and of the first button, relative to the top of the panel.
    int m_separatorTop = 0;
    int m_buttonsTop = 0;
    int m_height = MINIMUM_HEIGHT;

    std::chrono::steady_clock::time_point m_openedAt;
    float m_secondsOpen = 0.0f;
    // While the dialog opens, it fades in and rises from m_openingOffset below its place.
    float m_opacity = 1.0f;
    int m_openingOffset = 0;
    // The particles over the portrait; empty when the NPC has none.
    std::optional<UI::Quests::RisingParticles> m_portraitParticles;

    bool m_hasPortrait = false;
    // The NPC whose portrait is loaded; NoPortrait when none is.
    int m_portraitNpcNumber;
    // The NPCs which have no portrait, so that they aren't looked for again.
    std::set<uint16_t> m_npcsWithoutPortrait;
};
} // namespace SEASON3B
