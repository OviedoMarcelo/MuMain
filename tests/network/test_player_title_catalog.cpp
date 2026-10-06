#include "Core/Platform/WinCompat.h"
#include "Data/Translation/MultiLanguage.h"
#include "GameLogic/Social/PlayerTitleCatalog.h"

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
using GameLogic::Social::PlayerTitles;

constexpr size_t PacketSize = 44;
constexpr size_t PlayerIdOffset = 6;
constexpr size_t ColorOffset = 8;
constexpr size_t TextOffset = 12;

// A PlayerTitle message: big endian player id, little endian ARGB color, UTF-8 text.
std::vector<BYTE> MakePacket(WORD playerId, DWORD color, const std::string& text)
{
    std::vector<BYTE> packet(PacketSize);
    packet[PlayerIdOffset] = static_cast<BYTE>(playerId >> 8);
    packet[PlayerIdOffset + 1] = static_cast<BYTE>(playerId & 0xFF);
    for (size_t i = 0; i < sizeof(color); ++i)
    {
        packet[ColorOffset + i] = static_cast<BYTE>((color >> (i * 8)) & 0xFF);
    }

    std::memcpy(packet.data() + TextOffset, text.data(), text.size());
    return packet;
}

bool Add(const std::vector<BYTE>& packet)
{
    return PlayerTitles().AddFromPacket(packet.data(), static_cast<int32_t>(packet.size()));
}
} // namespace

TEST_CASE("player title catalog reads the title and its color")
{
    PlayerTitles().Reset();
    REQUIRE(Add(MakePacket(0x1234, 0xFFFFD700, "Lord of Arena")));

    const auto* title = PlayerTitles().Find(0x1234);
    REQUIRE(title != nullptr);
    CHECK(title->Text == L"Lord of Arena");
    CHECK(title->Red == 0xFF);
    CHECK(title->Green == 0xD7);
    CHECK(title->Blue == 0x00);
    CHECK(PlayerTitles().Find(0x1235) == nullptr);
}

TEST_CASE("player title catalog ignores the flag of a player which appears new")
{
    PlayerTitles().Reset();
    REQUIRE(Add(MakePacket(0x8000 | 0x0042, 0xFFFFFFFF, "Chaos Master")));

    CHECK(PlayerTitles().Find(0x0042) != nullptr);
}

TEST_CASE("player title catalog removes the title with an empty text")
{
    PlayerTitles().Reset();
    REQUIRE(Add(MakePacket(7, 0xFFFFFFFF, "Leyenda")));
    REQUIRE(Add(MakePacket(7, 0, "")));

    CHECK(PlayerTitles().Find(7) == nullptr);
}

TEST_CASE("player title catalog forgets a removed player")
{
    PlayerTitles().Reset();
    REQUIRE(Add(MakePacket(9, 0xFFFFFFFF, "Exterminador")));
    PlayerTitles().Remove(9);

    CHECK(PlayerTitles().Find(9) == nullptr);
}

TEST_CASE("player title catalog ignores a truncated message")
{
    PlayerTitles().Reset();
    auto packet = MakePacket(3, 0xFFFFFFFF, "Leyenda");
    packet.pop_back();

    CHECK_FALSE(Add(packet));
    CHECK(PlayerTitles().Find(3) == nullptr);
}
