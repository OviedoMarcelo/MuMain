#pragma once

#include "Core/Platform/WinCompat.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>

// The dialog of an NPC whose talk is a step of a quest: its portrait and name,
// what it says, and the buttons to accept or decline.
//
// We tell the server that we show these dialogs with a ClientFeatures message
// when the character entered the game. Then the server sends an NpcQuestDialog
// message instead of showing the text above the NPC, and the talk only counts
// when we answer that the player accepted. Older clients keep the text above
// the NPC, and their talk counts right away.
namespace GameLogic::Quests
{
struct NpcQuestDialog
{
    // The id of the dialog, which the answer refers to.
    uint32_t Id = 0;
    // The id of the NPC object, to find it in the viewport.
    uint16_t NpcKey = 0;
    // The number of the NPC definition, which chooses its portrait.
    uint16_t NpcNumber = 0;
    std::wstring NpcName;
    std::wstring QuestName;
    // What the NPC says. Lines are separated by a line feed.
    std::wstring Text;
};

// The size of the ClientFeatures message (C1 header with sub code).
inline constexpr int32_t ClientFeaturesPacketSize = 5;
// The size of the NpcQuestDialogAnswer message (C1 header with sub code).
inline constexpr int32_t AnswerPacketSize = 9;

std::array<BYTE, ClientFeaturesPacketSize> BuildClientFeaturesPacket();
std::array<BYTE, AnswerPacketSize> BuildAnswerPacket(uint32_t dialogId, bool accepted);

class NpcQuestDialogState
{
public:
    // Takes the dialog of an NpcQuestDialog message. It replaces the open one,
    // because the server only waits for the answer of the last one.
    bool AddFromPacket(const BYTE* data, int32_t size);

    // The dialog which is shown; nullptr when there is none.
    const NpcQuestDialog* GetOpened() const;

    // Changes whenever a dialog is opened or closed, so that the window knows when to lay it out again.
    uint32_t GetRevision() const
    {
        return m_revision;
    }

    // Closes the dialog without telling the server, e.g. when the character leaves the game.
    void Reset();

    // Tells the server that we show the dialogs. Implemented in NpcQuestDialogActions.cpp.
    static void AnnounceSupport();

    // Sends the answer to the server and closes the dialog. Implemented in NpcQuestDialogActions.cpp.
    void Answer(bool accepted);

    // The NPC is still in sight and close enough to answer it. Implemented in NpcQuestDialogActions.cpp.
    bool IsNpcInReach() const;

private:
    void Close();

    std::optional<NpcQuestDialog> m_opened;
    uint32_t m_revision = 0;
};

NpcQuestDialogState& NpcQuestDialogs();
} // namespace GameLogic::Quests
