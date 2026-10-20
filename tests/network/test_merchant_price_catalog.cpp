#include "Core/Platform/WinCompat.h"
#include "GameLogic/Items/MerchantPriceCatalog.h"

#include "doctest.h"

#include <vector>

namespace
{
using GameLogic::Items::MerchantPriceCatalog;
using GameLogic::Items::MerchantPrices;

constexpr size_t CountOffset = 6;
constexpr size_t EntriesOffset = 8;
constexpr size_t EntrySize = 9;

void WriteWord(std::vector<BYTE>& packet, size_t offset, WORD value)
{
    packet[offset] = static_cast<BYTE>(value & 0xFF);
    packet[offset + 1] = static_cast<BYTE>(value >> 8);
}

std::vector<BYTE> MakePacket(size_t count)
{
    std::vector<BYTE> packet(EntriesOffset + count * EntrySize);
    WriteWord(packet, CountOffset, static_cast<WORD>(count));
    return packet;
}

// A MerchantPrice entry: item group, level, number, price, and if it's the price of a piece.
void WritePrice(std::vector<BYTE>& packet, size_t index, BYTE group, BYTE level, WORD number, uint32_t price, bool isPerPiece = false)
{
    const auto offset = EntriesOffset + index * EntrySize;
    packet[offset + 0] = group;
    packet[offset + 1] = level;
    WriteWord(packet, offset + 2, number);
    WriteWord(packet, offset + 4, static_cast<WORD>(price & 0xFFFF));
    WriteWord(packet, offset + 6, static_cast<WORD>(price >> 16));
    packet[offset + 8] = isPerPiece ? 1 : 0;
}
} // namespace

TEST_CASE("merchant price catalog reads the prices by item and level")
{
    MerchantPrices().Reset();
    auto packet = MakePacket(2);
    WritePrice(packet, 0, 14, 1, 19, 1000000);
    WritePrice(packet, 1, 14, 7, 19, 7000000);

    REQUIRE(MerchantPrices().AddFromPacket(packet.data(), static_cast<int32_t>(packet.size())));

    CHECK(MerchantPrices().FindBuyingPrice(14, 19, 1) == 1000000);
    CHECK(MerchantPrices().FindBuyingPrice(14, 19, 7) == 7000000);
    CHECK_FALSE(MerchantPrices().FindBuyingPrice(14, 19, 2).has_value());
    CHECK_FALSE(MerchantPrices().FindBuyingPrice(13, 19, 1).has_value());
}

TEST_CASE("merchant price catalog multiplies the price of a piece by the pieces of a stack")
{
    MerchantPrices().Reset();
    auto packet = MakePacket(2);
    WritePrice(packet, 0, 14, 0, 3, 900000, true);
    WritePrice(packet, 1, 14, 1, 19, 1000000);

    REQUIRE(MerchantPrices().AddFromPacket(packet.data(), static_cast<int32_t>(packet.size())));

    CHECK(MerchantPrices().FindBuyingPrice(14, 3, 0, 3) == 2700000);
    CHECK(MerchantPrices().FindBuyingPrice(14, 3, 0, 1) == 900000);
    CHECK(MerchantPrices().FindBuyingPrice(14, 3, 0, 0) == 900000);
    CHECK(MerchantPrices().FindBuyingPrice(14, 19, 1, 5) == 1000000);
}

TEST_CASE("merchant price catalog replaces the prices of a previous message")
{
    MerchantPrices().Reset();
    auto first = MakePacket(1);
    WritePrice(first, 0, 14, 1, 19, 1000000);
    REQUIRE(MerchantPrices().AddFromPacket(first.data(), static_cast<int32_t>(first.size())));

    auto second = MakePacket(0);
    REQUIRE(MerchantPrices().AddFromPacket(second.data(), static_cast<int32_t>(second.size())));

    CHECK_FALSE(MerchantPrices().FindBuyingPrice(14, 19, 1).has_value());
}

TEST_CASE("merchant price catalog ignores a truncated message")
{
    MerchantPrices().Reset();
    auto packet = MakePacket(1);
    WritePrice(packet, 0, 14, 1, 19, 1000000);
    packet.pop_back();

    CHECK_FALSE(MerchantPrices().AddFromPacket(packet.data(), static_cast<int32_t>(packet.size())));
    CHECK_FALSE(MerchantPrices().FindBuyingPrice(14, 19, 1).has_value());
}

TEST_CASE("merchant price catalog sells for a third of the price, rounded like the server")
{
    CHECK(MerchantPriceCatalog::SellingPriceOf(1000000) == 333300);
    CHECK(MerchantPriceCatalog::SellingPriceOf(7000000) == 2333300);
    CHECK(MerchantPriceCatalog::SellingPriceOf(600) == 200);
}
