// NewUIQuestTracker.h: interface for the CNewUIQuestTracker class.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"
#include "UI/Quests/QuestTracker.h"

#include <cstdint>
#include <string>
#include <vector>

namespace SEASON3B
{
// Shows the chapter of the story and the quests in progress at the top right of
// the screen. It moves to the left of the party list when there is a party, and
// stays behind the windows which open at the right, like the inventory. A click
// on it opens the quest window.
class CNewUIQuestTracker : public CNewUIObj
{
    enum eLAYOUT
    {
        TRACKER_WIDTH = 150,
        TRACKER_TOP = 30,
        TRACKER_PADDING = 4,
        TRACKER_LINE_HEIGHT = 11,
        // The space between the tracker and the party list.
        TRACKER_PARTY_GAP = 4,
        TRACKER_MAX_QUESTS = 3,
    };

    struct Line
    {
        std::wstring Text;
        BYTE Red;
        BYTE Green;
        BYTE Blue;
        bool IsBold;
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
    // Lays out the lines again when the quests changed. It needs the font of the
    // text renderer to measure the lines, so it's done while rendering.
    void Layout();
    void AddLine(const std::wstring& text, BYTE red, BYTE green, BYTE blue, bool isBold);
    int GetLeft() const;
    int GetHeight() const;

    CNewUIManager* m_pNewUIMng = nullptr;
    std::vector<Line> m_lines;
    uint32_t m_layoutRevision = 0;
    bool m_isLaidOut = false;
};
} // namespace SEASON3B
