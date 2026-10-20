#pragma once

#include "Core/Platform/WinCompat.h"

#include <cstdint>
#include <optional>
#include <unordered_map>

// The prices which the merchant stores of the server set for their items.
//
// Without them we calculate the price of an item ourselves, as the server does by
// default. A store can set a price of its own for an item of a level, though; then
// the server sends it with the MerchantPrices message when a store is opened, and
// asks it when buying the item, and pays a third of it when selling it. For items
// which stack, like potions, it's the price of a piece: a stack costs it times its pieces.
namespace GameLogic::Items
{
class MerchantPriceCatalog
{
public:
    static MerchantPriceCatalog& Instance();

    // Forgets everything, e.g. when we connect to another server.
    void Reset();

    // Takes the MerchantPrices message, which replaces the prices of a previous one.
    // Returns false when the data isn't plausible, so that the caller can ignore it.
    bool AddFromPacket(const BYTE* data, int32_t size);

    // The buying price of the item of the group, number and level with the pieces, if a store sets it.
    std::optional<int64_t> FindBuyingPrice(int group, int number, int level, int pieces = 1) const;

    // The selling price of an item with the buying price, as the server pays it.
    static int64_t SellingPriceOf(int64_t buyingPrice);

private:
    struct StorePrice
    {
        int64_t Price;
        bool IsPerPiece;
    };

    std::unordered_map<uint32_t, StorePrice> m_prices;
};

inline MerchantPriceCatalog& MerchantPrices()
{
    return MerchantPriceCatalog::Instance();
}
} // namespace GameLogic::Items
