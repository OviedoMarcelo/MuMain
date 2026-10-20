#include "GameLogic/Items/MerchantPriceCatalog.h"

namespace GameLogic::Items
{
namespace
{
// The message has a C2 header with sub code, a count and a list of entries.
constexpr int32_t CountOffset = 6;
constexpr int32_t EntriesOffset = 8;
constexpr int32_t EntrySize = 9;

// Offsets within one MerchantPrice entry.
constexpr int32_t GroupOffset = 0;
constexpr int32_t LevelOffset = 1;
constexpr int32_t NumberOffset = 2;
constexpr int32_t PriceOffset = 4;
constexpr int32_t PerPieceOffset = 8;

WORD ReadWord(const BYTE* data, int32_t offset)
{
    return static_cast<WORD>(data[offset] | (data[offset + 1] << 8));
}

uint32_t ReadDword(const BYTE* data, int32_t offset)
{
    return static_cast<uint32_t>(ReadWord(data, offset)) | (static_cast<uint32_t>(ReadWord(data, offset + 2)) << 16);
}

uint32_t MakeKey(int group, int number, int level)
{
    return (static_cast<uint32_t>(group & 0xFF) << 24)
        | (static_cast<uint32_t>(number & 0xFFFF) << 8)
        | static_cast<uint32_t>(level & 0xFF);
}

// Like the server, which rounds the prices it calculates.
int64_t RoundPrice(int64_t price)
{
    if (price >= 1000)
    {
        return price / 100 * 100;
    }

    if (price >= 100)
    {
        return price / 10 * 10;
    }

    return price;
}
} // namespace

MerchantPriceCatalog& MerchantPriceCatalog::Instance()
{
    static MerchantPriceCatalog instance;
    return instance;
}

void MerchantPriceCatalog::Reset()
{
    this->m_prices.clear();
}

bool MerchantPriceCatalog::AddFromPacket(const BYTE* data, int32_t size)
{
    if (data == nullptr || size < EntriesOffset)
    {
        return false;
    }

    const int32_t count = ReadWord(data, CountOffset);
    if (size < EntriesOffset + count * EntrySize)
    {
        return false;
    }

    this->m_prices.clear();
    for (int32_t i = 0; i < count; ++i)
    {
        const auto* entry = data + EntriesOffset + i * EntrySize;
        const auto key = MakeKey(entry[GroupOffset], ReadWord(entry, NumberOffset), entry[LevelOffset]);
        this->m_prices[key] = StorePrice{ ReadDword(entry, PriceOffset), entry[PerPieceOffset] != 0 };
    }

    return true;
}

std::optional<int64_t> MerchantPriceCatalog::FindBuyingPrice(int group, int number, int level, int pieces) const
{
    const auto found = this->m_prices.find(MakeKey(group, number, level));
    if (found == this->m_prices.end())
    {
        return std::nullopt;
    }

    return found->second.IsPerPiece && pieces > 1 ? found->second.Price * pieces : found->second.Price;
}

int64_t MerchantPriceCatalog::SellingPriceOf(int64_t buyingPrice)
{
    return RoundPrice(buyingPrice / 3);
}
} // namespace GameLogic::Items
