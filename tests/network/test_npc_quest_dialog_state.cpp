#include "Core/Platform/WinCompat.h"
#include "Data/Translation/MultiLanguage.h"
#include "GameLogic/Quests/NpcQuestDialogState.h"

#include "doctest.h"

#include <algorithm>
#include <string>
#include <vector>

// The tests only use ASCII texts, so a byte is a character.
int32_t CMultiLanguage::ConvertFromUtf8(wchar_t* target, const char* source, int maxSourceLength)
{
    int32_t length = 0;
    while (length < maxSourceLength && source[length] != '\0')
    {
        target[length] = static_cast<unsigned char>(source[length]);
        ++length;
    }

    target[length] = L'\0';
    return length;
}

namespace
{
using GameLogic::Quests::NpcQuestDialogState;

constexpr size_t NpcKeyOffset = 5;
constexpr size_t NpcNumberOffset = 7;
constexpr size_t DialogIdOffset = 9;
constexpr size_t NpcNameOffset = 13;
constexpr size_t QuestNameOffset = 45;
constexpr size_t TextOffset = 93;

void WriteString(std::vector<BYTE>& packet, size_t offset, const std::string& text)
{
    std::copy(text.begin(), text.end(), packet.begin() + static_cast<std::ptrdiff_t>(offset));
}

// An NpcQuestDialog message: C2 F5 11, the NPC, the dialog id, the names and the text with its terminating zero.
std::vector<BYTE> MakeDialogPacket(uint32_t dialogId, const std::string& text)
{
    std::vector<BYTE> packet(TextOffset + text.size() + 1);
    packet[0] = 0xC2;
    packet[3] = 0xF5;
    packet[4] = 0x11;
    packet[NpcKeyOffset] = 0x80; // The flag in the highest bit isn't part of the key.
    packet[NpcKeyOffset + 1] = 0x2A;
    packet[NpcNumberOffset] = 0xF0; // 240, little endian
    packet[DialogIdOffset] = static_cast<BYTE>(dialogId);
    WriteString(packet, NpcNameOffset, "Elf Lala");
    WriteString(packet, QuestNameOffset, "Noria");
    WriteString(packet, TextOffset, text);
    return packet;
}
} // namespace

TEST_CASE("An NpcQuestDialog message opens the dialog")
{
    NpcQuestDialogState state;
    const auto revision = state.GetRevision();
    auto packet = MakeDialogPacket(7, "Hola.\nLa corrupcion llego a Noria.");

    REQUIRE(state.AddFromPacket(packet.data(), static_cast<int32_t>(packet.size())));

    const auto* dialog = state.GetOpened();
    REQUIRE(dialog != nullptr);
    CHECK(dialog->Id == 7);
    CHECK(dialog->NpcKey == 0x2A);
    CHECK(dialog->NpcNumber == 240);
    CHECK(dialog->NpcName == L"Elf Lala");
    CHECK(dialog->QuestName == L"Noria");
    CHECK(dialog->Text == L"Hola.\nLa corrupcion llego a Noria.");
    CHECK(state.GetRevision() != revision);
}

TEST_CASE("A new dialog replaces the open one, and a reset closes it")
{
    NpcQuestDialogState state;
    auto first = MakeDialogPacket(1, "Uno");
    auto second = MakeDialogPacket(2, "Dos");
    state.AddFromPacket(first.data(), static_cast<int32_t>(first.size()));
    state.AddFromPacket(second.data(), static_cast<int32_t>(second.size()));

    REQUIRE(state.GetOpened() != nullptr);
    CHECK(state.GetOpened()->Id == 2);

    const auto revision = state.GetRevision();
    state.Reset();
    CHECK(state.GetOpened() == nullptr);
    CHECK(state.GetRevision() != revision);
}

TEST_CASE("A message which is too short is ignored")
{
    NpcQuestDialogState state;
    std::vector<BYTE> packet(TextOffset);

    CHECK_FALSE(state.AddFromPacket(packet.data(), static_cast<int32_t>(packet.size())));
    CHECK_FALSE(state.AddFromPacket(nullptr, 0));
    CHECK(state.GetOpened() == nullptr);
}

TEST_CASE("The answer and the features are C1 F5 messages")
{
    const auto features = GameLogic::Quests::BuildClientFeaturesPacket();
    CHECK(std::vector<BYTE>(features.begin(), features.end()) == std::vector<BYTE>{0xC1, 0x05, 0xF5, 0x01, 0x01});

    const auto accepted = GameLogic::Quests::BuildAnswerPacket(0x01020304, true);
    CHECK(std::vector<BYTE>(accepted.begin(), accepted.end()) ==
          std::vector<BYTE>{0xC1, 0x09, 0xF5, 0x02, 0x04, 0x03, 0x02, 0x01, 0x01});

    const auto declined = GameLogic::Quests::BuildAnswerPacket(5, false);
    CHECK(declined[8] == 0);
}
