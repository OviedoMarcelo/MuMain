#include "GameLogic/Social/PlayerTitleCatalog.h"

#include "Data/Translation/MultiLanguage.h"

#include <vector>

namespace GameLogic::Social
{
namespace
{
// Offsets of the PlayerTitle message (C2 header with sub code).
constexpr int32_t PacketSize = 44;
constexpr int32_t PlayerIdOffset = 6;
constexpr int32_t ColorOffset = 8;
constexpr int32_t TextOffset = 12;
constexpr int32_t TextLength = 32;

// The highest bit of a player id only marks a player which appears new.
constexpr int KeyMask = 0x7FFF;

// The player id is big endian, like in the AddCharactersToScope message.
int ReadKey(const BYTE* data)
{
    return ((data[PlayerIdOffset] << 8) | data[PlayerIdOffset + 1]) & KeyMask;
}

// The color is a little endian ARGB value.
PlayerTitle ReadColor(const BYTE* data)
{
    PlayerTitle title;
    title.Blue = data[ColorOffset];
    title.Green = data[ColorOffset + 1];
    title.Red = data[ColorOffset + 2];
    return title;
}

// The text is UTF-8 and padded with zeros.
std::wstring ReadText(const BYTE* data)
{
    std::vector<wchar_t> buffer(static_cast<size_t>(TextLength) + 1, L'\0');
    CMultiLanguage::ConvertFromUtf8(buffer.data(), reinterpret_cast<const char*>(data + TextOffset), TextLength);
    buffer[TextLength] = L'\0';
    return std::wstring(buffer.data());
}
} // namespace

PlayerTitleCatalog& PlayerTitleCatalog::Instance()
{
    static PlayerTitleCatalog instance;
    return instance;
}

void PlayerTitleCatalog::Reset()
{
    this->m_titles.clear();
}

bool PlayerTitleCatalog::AddFromPacket(const BYTE* data, int32_t size)
{
    if (data == nullptr || size < PacketSize)
    {
        return false;
    }

    const int key = ReadKey(data);
    auto title = ReadColor(data);
    title.Text = ReadText(data);
    if (title.Text.empty())
    {
        this->m_titles.erase(key);
        return true;
    }

    this->m_titles[key] = std::move(title);
    return true;
}

void PlayerTitleCatalog::Remove(int key)
{
    this->m_titles.erase(key & KeyMask);
}

const PlayerTitle* PlayerTitleCatalog::Find(int key) const
{
    const auto found = this->m_titles.find(key & KeyMask);
    return found == this->m_titles.end() ? nullptr : &found->second;
}
} // namespace GameLogic::Social
