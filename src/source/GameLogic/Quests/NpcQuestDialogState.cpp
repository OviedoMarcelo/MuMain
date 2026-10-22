#include "GameLogic/Quests/NpcQuestDialogState.h"

#include "Network/PacketFields.h"

namespace GameLogic::Quests
{
namespace
{
using namespace Network::PacketFields;

constexpr BYTE C1Header = 0xC1;
constexpr BYTE GroupCode = 0xF5;
constexpr int32_t SizeOffset = 1;
constexpr int32_t CodeOffset = 2;
constexpr int32_t SubCodeOffset = 3;

// The ClientFeatures message (C1 F5 01) with its flags.
constexpr BYTE ClientFeaturesSubCode = 0x01;
constexpr int32_t FeaturesOffset = 4;
constexpr BYTE NpcQuestDialogFeature = 0x01;

// The NpcQuestDialogAnswer message (C1 F5 02).
constexpr BYTE AnswerSubCode = 0x02;
constexpr int32_t AnswerDialogIdOffset = 4;
constexpr int32_t AnswerAcceptedOffset = 8;

// Offsets of the NpcQuestDialog message (C2 header with sub code). The text takes the rest of it.
constexpr int32_t NpcKeyOffset = 5;
constexpr int32_t NpcNumberOffset = 7;
constexpr int32_t DialogIdOffset = 9;
constexpr int32_t NpcNameOffset = 13;
constexpr int32_t NpcNameLength = 32;
constexpr int32_t QuestNameOffset = NpcNameOffset + NpcNameLength;
constexpr int32_t QuestNameLength = 48;
constexpr int32_t TextOffset = QuestNameOffset + QuestNameLength;
// The highest bit of an object id is a flag, like in the viewport messages.
constexpr uint16_t ObjectKeyMask = 0x7FFF;

template <size_t Size>
std::array<BYTE, Size> CreatePacket(BYTE subCode)
{
    std::array<BYTE, Size> packet{};
    packet[0] = C1Header;
    packet[SizeOffset] = static_cast<BYTE>(Size);
    packet[CodeOffset] = GroupCode;
    packet[SubCodeOffset] = subCode;
    return packet;
}

NpcQuestDialog ReadDialog(const BYTE* data, int32_t size)
{
    NpcQuestDialog dialog;
    dialog.Id = ReadUInt32LittleEndian(data, DialogIdOffset);
    dialog.NpcKey = ReadUInt16BigEndian(data, NpcKeyOffset) & ObjectKeyMask;
    dialog.NpcNumber = ReadUInt16LittleEndian(data, NpcNumberOffset);
    dialog.NpcName = ReadString(data, NpcNameOffset, NpcNameLength);
    dialog.QuestName = ReadString(data, QuestNameOffset, QuestNameLength);
    dialog.Text = ReadString(data, TextOffset, size - TextOffset);
    return dialog;
}
} // namespace

std::array<BYTE, ClientFeaturesPacketSize> BuildClientFeaturesPacket()
{
    auto packet = CreatePacket<ClientFeaturesPacketSize>(ClientFeaturesSubCode);
    packet[FeaturesOffset] = NpcQuestDialogFeature;
    return packet;
}

std::array<BYTE, AnswerPacketSize> BuildAnswerPacket(uint32_t dialogId, bool accepted)
{
    auto packet = CreatePacket<AnswerPacketSize>(AnswerSubCode);
    WriteUInt32LittleEndian(packet.data(), AnswerDialogIdOffset, dialogId);
    packet[AnswerAcceptedOffset] = accepted ? 1 : 0;
    return packet;
}

bool NpcQuestDialogState::AddFromPacket(const BYTE* data, int32_t size)
{
    if (data == nullptr || size <= TextOffset)
    {
        return false;
    }

    m_opened = ReadDialog(data, size);
    ++m_revision;
    return true;
}

const NpcQuestDialog* NpcQuestDialogState::GetOpened() const
{
    return m_opened ? &*m_opened : nullptr;
}

void NpcQuestDialogState::Reset()
{
    Close();
}

void NpcQuestDialogState::Close()
{
    if (!m_opened)
    {
        return;
    }

    m_opened.reset();
    ++m_revision;
}

NpcQuestDialogState& NpcQuestDialogs()
{
    static NpcQuestDialogState state;
    return state;
}
} // namespace GameLogic::Quests
