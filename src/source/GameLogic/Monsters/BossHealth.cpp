#include "GameLogic/Monsters/BossHealth.h"

#include "Data/Translation/MultiLanguage.h"

#include <vector>

namespace GameLogic::Monsters
{
namespace
{
// Offsets of the BossHealth message (C2 header with sub code).
constexpr int32_t PacketSize = 76;
constexpr int32_t StateOffset = 5;
constexpr int32_t ObjectIdOffset = 6;
constexpr int32_t HealthOffset = 8;
constexpr int32_t BossNameOffset = 12;
constexpr int32_t PhaseNameOffset = 44;
constexpr int32_t NameLength = 32;

constexpr BYTE HiddenState = 0;
constexpr int MaximumHealthPercent = 100;

// The highest bit of an object id only marks a monster which appears new.
constexpr int KeyMask = 0x7FFF;

// The object id is big endian, like in the AddNpcsToScope message.
int ReadKey(const BYTE* data)
{
    return ((data[ObjectIdOffset] << 8) | data[ObjectIdOffset + 1]) & KeyMask;
}

// The names are UTF-8 and padded with zeros.
std::wstring ReadName(const BYTE* data, int32_t offset)
{
    std::vector<wchar_t> buffer(static_cast<size_t>(NameLength) + 1, L'\0');
    CMultiLanguage::ConvertFromUtf8(buffer.data(), reinterpret_cast<const char*>(data + offset), NameLength);
    buffer[NameLength] = L'\0';
    return std::wstring(buffer.data());
}
} // namespace

std::optional<BossHealth> ReadBossHealth(const BYTE* data, int32_t size)
{
    if (data == nullptr || size < PacketSize)
    {
        return std::nullopt;
    }

    BossHealth health;
    health.Key = ReadKey(data);
    health.IsVisible = data[StateOffset] != HiddenState;
    if (!health.IsVisible)
    {
        return health;
    }

    const int healthPercent = data[HealthOffset];
    health.HealthPercent = healthPercent > MaximumHealthPercent ? MaximumHealthPercent : healthPercent;
    health.BossName = ReadName(data, BossNameOffset);
    health.PhaseName = ReadName(data, PhaseNameOffset);
    return health;
}
} // namespace GameLogic::Monsters
