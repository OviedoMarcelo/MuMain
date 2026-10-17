#include "Core/Platform/WinCompat.h"
#include "Data/Translation/MultiLanguage.h"
#include "GameLogic/Cosmetics/CosmeticSetCatalog.h"

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
using GameLogic::Cosmetics::CosmeticSets;

constexpr size_t HeaderSize = 8;
constexpr size_t SetSize = 40;

struct TestSet
{
    WORD Number;
    WORD ItemNumber;
    BYTE Level;
    bool IsExcellent;
    bool IsUnlocked;
    bool CanWear;
    std::string Name;
};

void WriteWord(std::vector<BYTE>& packet, size_t offset, WORD value)
{
    packet[offset] = static_cast<BYTE>(value & 0xFF);
    packet[offset + 1] = static_cast<BYTE>(value >> 8);
}

// A CosmeticSets message: the number of the worn set and the sets.
std::vector<BYTE> MakePacket(WORD wornSetNumber, const std::vector<TestSet>& sets)
{
    std::vector<BYTE> packet(HeaderSize + sets.size() * SetSize);
    packet[5] = static_cast<BYTE>(sets.size());
    WriteWord(packet, 6, wornSetNumber);
    for (size_t i = 0; i < sets.size(); ++i)
    {
        const size_t entry = HeaderSize + i * SetSize;
        WriteWord(packet, entry, sets[i].Number);
        WriteWord(packet, entry + 2, sets[i].ItemNumber);
        packet[entry + 4] = sets[i].Level;
        packet[entry + 5] = sets[i].IsExcellent ? 1 : 0;
        packet[entry + 6] = sets[i].IsUnlocked ? 1 : 0;
        packet[entry + 7] = sets[i].CanWear ? 1 : 0;
        std::memcpy(packet.data() + entry + 8, sets[i].Name.data(), sets[i].Name.size());
    }

    return packet;
}

const std::vector<TestSet> TwoSets = {
    { 1, 1, 9, false, true, true, "Dragon" },
    { 3, 16, 11, true, false, true, "Black Dragon" },
};
} // namespace

TEST_CASE("CosmeticSetCatalog reads the sets of the character")
{
    CosmeticSets().Reset();
    const auto packet = MakePacket(1, TwoSets);

    REQUIRE(CosmeticSets().AddFromPacket(packet.data(), static_cast<int32_t>(packet.size())));

    CHECK(CosmeticSets().IsAvailable());
    const auto& sets = CosmeticSets().GetSets();
    REQUIRE(sets.size() == 2);
    CHECK(sets[1].Number == 3);
    CHECK(sets[1].ItemNumber == 16);
    CHECK(sets[1].Level == 11);
    CHECK(sets[1].IsExcellent);
    CHECK_FALSE(sets[1].IsUnlocked);
    CHECK(sets[1].CanWear);
    CHECK(sets[1].Name == L"Black Dragon");
}

TEST_CASE("CosmeticSetCatalog knows the worn set")
{
    CosmeticSets().Reset();
    const auto packet = MakePacket(3, TwoSets);
    REQUIRE(CosmeticSets().AddFromPacket(packet.data(), static_cast<int32_t>(packet.size())));

    const auto* worn = CosmeticSets().GetWornSet();
    REQUIRE(worn != nullptr);
    CHECK(worn->Name == L"Black Dragon");

    const auto none = MakePacket(0, TwoSets);
    REQUIRE(CosmeticSets().AddFromPacket(none.data(), static_cast<int32_t>(none.size())));
    CHECK(CosmeticSets().GetWornSet() == nullptr);
}

TEST_CASE("CosmeticSetCatalog ignores a truncated message")
{
    CosmeticSets().Reset();
    auto packet = MakePacket(1, TwoSets);
    packet.resize(packet.size() - 1);

    CHECK_FALSE(CosmeticSets().AddFromPacket(packet.data(), static_cast<int32_t>(packet.size())));
    CHECK_FALSE(CosmeticSets().IsAvailable());
    CHECK(CosmeticSets().GetWornSet() == nullptr);
}

TEST_CASE("CosmeticSetCatalog forgets the sets and counts the revision on reset")
{
    CosmeticSets().Reset();
    const auto packet = MakePacket(1, TwoSets);
    REQUIRE(CosmeticSets().AddFromPacket(packet.data(), static_cast<int32_t>(packet.size())));
    const auto revision = CosmeticSets().GetRevision();

    CosmeticSets().Reset();

    CHECK_FALSE(CosmeticSets().IsAvailable());
    CHECK(CosmeticSets().GetSets().empty());
    CHECK(CosmeticSets().GetWornSet() == nullptr);
    CHECK(CosmeticSets().GetRevision() > revision);
}
