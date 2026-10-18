#include "Core/Platform/WinCompat.h"
#include "Data/Translation/MultiLanguage.h"
#include "GameLogic/Cosmetics/CosmeticPieceCatalog.h"

#include "doctest.h"

#include <cstring>
#include <string>
#include <vector>

// The texts of these tests are ASCII, so the client's UTF-8 conversion isn't needed.
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
using GameLogic::Cosmetics::CosmeticPieces;

constexpr size_t HeaderSize = 8;
constexpr size_t PieceSize = 42;
constexpr BYTE HelmGroup = 7;
constexpr BYTE ArmorGroup = 8;

struct TestPiece
{
    WORD Number;
    WORD ItemNumber;
    BYTE ItemGroup;
    BYTE Level;
    bool IsExcellent;
    bool IsUnlocked;
    bool CanWear;
    bool IsWorn;
    std::string Name;
};

void WriteWord(std::vector<BYTE>& packet, size_t offset, WORD value)
{
    packet[offset] = static_cast<BYTE>(value & 0xFF);
    packet[offset + 1] = static_cast<BYTE>(value >> 8);
}

// A CosmeticPieces message with the pieces.
std::vector<BYTE> MakePacket(const std::vector<TestPiece>& pieces)
{
    std::vector<BYTE> packet(HeaderSize + pieces.size() * PieceSize);
    WriteWord(packet, 6, static_cast<WORD>(pieces.size()));
    for (size_t i = 0; i < pieces.size(); ++i)
    {
        const size_t entry = HeaderSize + i * PieceSize;
        WriteWord(packet, entry, pieces[i].Number);
        WriteWord(packet, entry + 2, pieces[i].ItemNumber);
        packet[entry + 4] = pieces[i].ItemGroup;
        packet[entry + 5] = pieces[i].Level;
        packet[entry + 6] = pieces[i].IsExcellent ? 1 : 0;
        packet[entry + 7] = pieces[i].IsUnlocked ? 1 : 0;
        packet[entry + 8] = pieces[i].CanWear ? 1 : 0;
        packet[entry + 9] = pieces[i].IsWorn ? 1 : 0;
        std::memcpy(packet.data() + entry + 10, pieces[i].Name.data(), pieces[i].Name.size());
    }

    return packet;
}

const std::vector<TestPiece> ThreePieces = {
    {11, 1, HelmGroup, 9, false, true, true, false, "Dragon Helm"},
    {21, 16, HelmGroup, 11, true, true, true, true, "Black Dragon Helm"},
    {12, 1, ArmorGroup, 9, false, false, true, false, "Dragon Armor"},
};
} // namespace

TEST_CASE("CosmeticPieceCatalog reads the pieces of the character")
{
    CosmeticPieces().Reset();
    const auto packet = MakePacket(ThreePieces);

    REQUIRE(CosmeticPieces().AddFromPacket(packet.data(), static_cast<int32_t>(packet.size())));

    CHECK(CosmeticPieces().IsAvailable());
    const auto& pieces = CosmeticPieces().GetPieces();
    REQUIRE(pieces.size() == 3);
    CHECK(pieces[1].Number == 21);
    CHECK(pieces[1].ItemNumber == 16);
    CHECK(pieces[1].ItemGroup == HelmGroup);
    CHECK(pieces[1].Level == 11);
    CHECK(pieces[1].IsExcellent);
    CHECK(pieces[1].IsWorn);
    CHECK_FALSE(pieces[2].IsUnlocked);
    CHECK(pieces[1].Name == L"Black Dragon Helm");
}

TEST_CASE("CosmeticPieceCatalog knows the worn piece of each slot")
{
    CosmeticPieces().Reset();
    const auto packet = MakePacket(ThreePieces);
    REQUIRE(CosmeticPieces().AddFromPacket(packet.data(), static_cast<int32_t>(packet.size())));

    const auto* helm = CosmeticPieces().GetWornPiece(HelmGroup);
    REQUIRE(helm != nullptr);
    CHECK(helm->Number == 21);
    CHECK(CosmeticPieces().GetWornPiece(ArmorGroup) == nullptr);
}

TEST_CASE("CosmeticPieceCatalog ignores a truncated message")
{
    CosmeticPieces().Reset();
    auto packet = MakePacket(ThreePieces);
    packet.resize(packet.size() - 1);

    CHECK_FALSE(CosmeticPieces().AddFromPacket(packet.data(), static_cast<int32_t>(packet.size())));
    CHECK_FALSE(CosmeticPieces().IsAvailable());
    CHECK(CosmeticPieces().GetPieces().empty());
}

TEST_CASE("CosmeticPieceCatalog forgets the pieces and counts the revision on reset")
{
    CosmeticPieces().Reset();
    const auto packet = MakePacket(ThreePieces);
    REQUIRE(CosmeticPieces().AddFromPacket(packet.data(), static_cast<int32_t>(packet.size())));
    const auto revision = CosmeticPieces().GetRevision();

    CosmeticPieces().Reset();

    CHECK_FALSE(CosmeticPieces().IsAvailable());
    CHECK(CosmeticPieces().GetPieces().empty());
    CHECK(CosmeticPieces().GetWornPiece(HelmGroup) == nullptr);
    CHECK(CosmeticPieces().GetRevision() > revision);
}
