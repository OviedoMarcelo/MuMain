#include "Core/Platform/WinCompat.h"
#include "Data/Translation/MultiLanguage.h"
#include "GameLogic/Events/EventScheduleCatalog.h"

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
using GameLogic::Events::EventSchedule;
using GameLogic::Events::EventScheduleCatalog;
using GameLogic::Events::ScheduledEvent;

constexpr size_t TimeZoneNameOffset = 46;
constexpr size_t StartsOffset = 70;

// An EventScheduleEntry message: little endian numbers, UTF-8 name, the starts as little endian minutes.
std::vector<BYTE> MakePacket(BYTE index, BYTE count, const std::string& name, int durationMinutes, int serverSecondOfDay,
                             const std::vector<int>& starts, const std::string& timeZoneName = "Argentina")
{
    std::vector<BYTE> packet(StartsOffset + starts.size() * 2);
    packet[5] = index;
    packet[6] = count;
    packet[7] = static_cast<BYTE>(starts.size());
    packet[8] = static_cast<BYTE>(durationMinutes & 0xFF);
    packet[9] = static_cast<BYTE>(durationMinutes >> 8);
    for (int i = 0; i < 4; ++i)
    {
        packet[10 + i] = static_cast<BYTE>((serverSecondOfDay >> (i * 8)) & 0xFF);
    }

    std::memcpy(packet.data() + 14, name.data(), name.size());
    std::memcpy(packet.data() + TimeZoneNameOffset, timeZoneName.data(), timeZoneName.size());
    for (size_t i = 0; i < starts.size(); ++i)
    {
        packet[StartsOffset + i * 2] = static_cast<BYTE>(starts[i] & 0xFF);
        packet[StartsOffset + i * 2 + 1] = static_cast<BYTE>(starts[i] >> 8);
    }

    return packet;
}

bool Add(const std::vector<BYTE>& packet, uint64_t receivedAtMs = 0)
{
    return EventSchedule().AddFromPacket(packet.data(), static_cast<int32_t>(packet.size()), receivedAtMs);
}

ScheduledEvent MakeEvent(int durationMinutes, std::vector<int> starts)
{
    ScheduledEvent scheduledEvent;
    scheduledEvent.DurationMinutes = durationMinutes;
    scheduledEvent.StartMinutes = std::move(starts);
    return scheduledEvent;
}
} // namespace

TEST_CASE("event schedule catalog reads a complete list")
{
    EventSchedule().Reset();
    REQUIRE(Add(MakePacket(0, 2, "Blood Castle", 20, 3600, {120, 0})));
    CHECK_FALSE(EventSchedule().IsAvailable());
    REQUIRE(Add(MakePacket(1, 2, "Kanturu", 135, 3600, {1200})));
    REQUIRE(EventSchedule().IsAvailable());

    const auto& events = EventSchedule().GetEvents();
    REQUIRE(events.size() == 2);
    CHECK(events[0].Name == L"Blood Castle");
    CHECK(events[0].DurationMinutes == 20);
    CHECK(events[0].StartMinutes == std::vector<int>{0, 120});
    CHECK(events[1].Name == L"Kanturu");
    CHECK(EventSchedule().GetTimeZoneName() == L"Argentina");
}

TEST_CASE("event schedule catalog replaces the list which is sent again")
{
    EventSchedule().Reset();
    REQUIRE(Add(MakePacket(0, 1, "Blood Castle", 20, 0, {0})));
    REQUIRE(Add(MakePacket(0, 1, "Devil Square", 20, 0, {60})));
    REQUIRE(EventSchedule().GetEvents().size() == 1);
    CHECK(EventSchedule().GetEvents()[0].Name == L"Devil Square");

    REQUIRE(Add(MakePacket(0, 0, "", 0, 0, {})));
    CHECK(EventSchedule().IsAvailable());
    CHECK(EventSchedule().GetEvents().empty());
}

TEST_CASE("event schedule catalog rejects implausible messages")
{
    EventSchedule().Reset();
    auto truncated = MakePacket(0, 1, "Blood Castle", 20, 0, {0, 120});
    truncated.resize(truncated.size() - 1);
    CHECK_FALSE(Add(truncated));
    CHECK_FALSE(Add(MakePacket(1, 2, "Out of order", 20, 0, {0})));
    CHECK_FALSE(Add(MakePacket(0, 1, "Wrong time", 20, EventScheduleCatalog::SecondsPerDay, {0})));
}

TEST_CASE("event schedule catalog counts the time of the server on")
{
    EventSchedule().Reset();
    REQUIRE(Add(MakePacket(0, 1, "Blood Castle", 20, EventScheduleCatalog::SecondsPerDay - 10, {0}), 5000));
    CHECK(EventSchedule().GetServerSecondOfDay(5000) == EventScheduleCatalog::SecondsPerDay - 10);
    CHECK(EventSchedule().GetServerSecondOfDay(25000) == 10);
}

TEST_CASE("event state tells the next start and running events")
{
    const auto bloodCastle = MakeEvent(20, {0, 120});

    // At 01:00 the next start is at 02:00.
    auto state = EventScheduleCatalog::GetState(bloodCastle, 3600);
    CHECK_FALSE(state.IsRunning);
    CHECK(state.StartMinute == 120);
    CHECK(state.SecondsLeft == 3600);

    // At 02:05 it runs for 15 more minutes.
    state = EventScheduleCatalog::GetState(bloodCastle, 2 * 3600 + 300);
    CHECK(state.IsRunning);
    CHECK(state.StartMinute == 120);
    CHECK(state.SecondsLeft == 900);

    // At 23:00 the next start is tomorrow at 00:00.
    state = EventScheduleCatalog::GetState(bloodCastle, 23 * 3600);
    CHECK_FALSE(state.IsRunning);
    CHECK(state.StartMinute == 0);
    CHECK(state.SecondsLeft == 3600);

    // An event which started at 23:50 still runs after midnight.
    state = EventScheduleCatalog::GetState(MakeEvent(30, {23 * 60 + 50}), 300);
    CHECK(state.IsRunning);
    CHECK(state.SecondsLeft == 900);
}
