#include "GameLogic/Monsters/MonsterAppearance.h"

#include "Data/Translation/MultiLanguage.h"

#include <vector>

namespace GameLogic::Monsters
{
namespace
{
// Offsets of the MonsterAppearance message (C2 header with sub code).
constexpr int32_t PacketSize = 44;
constexpr int32_t GlowOffset = 5;
constexpr int32_t ObjectIdOffset = 6;
constexpr int32_t ScaleOffset = 8;
constexpr int32_t NameOffset = 12;
constexpr int32_t NameLength = 32;

// The highest bit of an object id only marks a monster which appears new.
constexpr int KeyMask = 0x7FFF;

// The object id is big endian, like in the AddNpcsToScope message.
int ReadKey(const BYTE* data)
{
    return ((data[ObjectIdOffset] << 8) | data[ObjectIdOffset + 1]) & KeyMask;
}

// The scale is little endian.
int ReadScalePercent(const BYTE* data)
{
    return data[ScaleOffset] | (data[ScaleOffset + 1] << 8);
}

MonsterGlow ReadGlow(const BYTE* data)
{
    const auto glow = static_cast<MonsterGlow>(data[GlowOffset]);
    switch (glow)
    {
    case MonsterGlow::Level7:
    case MonsterGlow::Level9:
    case MonsterGlow::Level11:
    case MonsterGlow::Level13:
    case MonsterGlow::Level15:
    case MonsterGlow::Golden:
        return glow;
    default:
        return MonsterGlow::None;
    }
}

// The name is UTF-8 and padded with zeros.
std::wstring ReadName(const BYTE* data)
{
    std::vector<wchar_t> buffer(static_cast<size_t>(NameLength) + 1, L'\0');
    CMultiLanguage::ConvertFromUtf8(buffer.data(), reinterpret_cast<const char*>(data + NameOffset), NameLength);
    buffer[NameLength] = L'\0';
    return std::wstring(buffer.data());
}
} // namespace

std::optional<MonsterAppearance> ReadMonsterAppearance(const BYTE* data, int32_t size)
{
    if (data == nullptr || size < PacketSize)
    {
        return std::nullopt;
    }

    const int scalePercent = ReadScalePercent(data);
    if (scalePercent == 0)
    {
        return std::nullopt;
    }

    MonsterAppearance appearance;
    appearance.Key = ReadKey(data);
    appearance.ScalePercent = scalePercent;
    appearance.Glow = ReadGlow(data);
    appearance.Name = ReadName(data);
    return appearance;
}
} // namespace GameLogic::Monsters
