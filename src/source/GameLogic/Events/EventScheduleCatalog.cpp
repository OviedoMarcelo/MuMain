#include "GameLogic/Events/EventScheduleCatalog.h"

#include "Data/Translation/MultiLanguage.h"

#include <algorithm>

namespace GameLogic::Events
{
namespace
{
// Offsets of the EventScheduleEntry message (C2 header with sub code).
constexpr int32_t IndexOffset = 5;
constexpr int32_t CountOffset = 6;
constexpr int32_t StartCountOffset = 7;
constexpr int32_t DurationOffset = 8;
constexpr int32_t ServerSecondOffset = 10;
constexpr int32_t NameOffset = 14;
constexpr int32_t NameLength = 32;
constexpr int32_t TimeZoneNameOffset = 46;
constexpr int32_t TimeZoneNameLength = 24;
constexpr int32_t StartsOffset = 70;
constexpr int32_t StartSize = 2;

constexpr int MinutesPerDay = 24 * 60;

int ReadShortLittleEndian(const BYTE* data, int32_t offset)
{
    return data[offset] | (data[offset + 1] << 8);
}

int ReadIntLittleEndian(const BYTE* data, int32_t offset)
{
    return static_cast<int>(static_cast<uint32_t>(data[offset]) | (static_cast<uint32_t>(data[offset + 1]) << 8) |
                            (static_cast<uint32_t>(data[offset + 2]) << 16) |
                            (static_cast<uint32_t>(data[offset + 3]) << 24));
}

// The strings are UTF-8 and padded with zeros.
std::wstring ReadString(const BYTE* data, int32_t offset, int32_t length)
{
    std::vector<wchar_t> buffer(static_cast<size_t>(length) + 1, L'\0');
    CMultiLanguage::ConvertFromUtf8(buffer.data(), reinterpret_cast<const char*>(data + offset), length);
    buffer[length] = L'\0';
    return std::wstring(buffer.data());
}
} // namespace

EventScheduleCatalog& EventScheduleCatalog::Instance()
{
    static EventScheduleCatalog instance;
    return instance;
}

void EventScheduleCatalog::Reset()
{
    m_events.clear();
    m_expectedCount = 0;
    m_isComplete = false;
    m_serverSecondOfDay = 0;
    m_receivedAtMs = 0;
    m_timeZoneName.clear();
}

bool EventScheduleCatalog::AddFromPacket(const BYTE* data, int32_t size, uint64_t receivedAtMs)
{
    if (data == nullptr || size < StartsOffset)
    {
        return false;
    }

    const auto startCount = data[StartCountOffset];
    if (size < StartsOffset + startCount * StartSize)
    {
        return false;
    }

    const auto serverSecondOfDay = ReadIntLittleEndian(data, ServerSecondOffset);
    if (serverSecondOfDay < 0 || serverSecondOfDay >= SecondsPerDay)
    {
        return false;
    }

    const auto index = data[IndexOffset];
    const auto count = data[CountOffset];

    // An empty list comes as one message without an event.
    if (count == 0)
    {
        m_events.clear();
        m_expectedCount = 0;
        m_isComplete = true;
        m_serverSecondOfDay = serverSecondOfDay;
        m_receivedAtMs = receivedAtMs;
        m_timeZoneName = ReadString(data, TimeZoneNameOffset, TimeZoneNameLength);
        return true;
    }

    if (index >= count)
    {
        return false;
    }

    // The first message of a list starts a new one, so that the list which is
    // sent again after a change replaces the old one.
    if (index == 0)
    {
        m_events.clear();
        m_expectedCount = count;
        m_isComplete = false;
    }
    else if (m_isComplete || count != m_expectedCount || index != m_events.size())
    {
        return false;
    }

    ScheduledEvent scheduledEvent;
    scheduledEvent.Name = ReadString(data, NameOffset, NameLength);
    scheduledEvent.DurationMinutes = ReadShortLittleEndian(data, DurationOffset);
    for (int i = 0; i < startCount; ++i)
    {
        const auto minute = ReadShortLittleEndian(data, StartsOffset + i * StartSize);
        if (minute < MinutesPerDay)
        {
            scheduledEvent.StartMinutes.push_back(minute);
        }
    }

    std::sort(scheduledEvent.StartMinutes.begin(), scheduledEvent.StartMinutes.end());
    scheduledEvent.StartMinutes.erase(std::unique(scheduledEvent.StartMinutes.begin(), scheduledEvent.StartMinutes.end()),
                                      scheduledEvent.StartMinutes.end());
    m_events.push_back(std::move(scheduledEvent));

    // The time of the server is taken from the last message, it's the most recent one.
    m_serverSecondOfDay = serverSecondOfDay;
    m_receivedAtMs = receivedAtMs;
    m_timeZoneName = ReadString(data, TimeZoneNameOffset, TimeZoneNameLength);
    m_isComplete = m_events.size() == m_expectedCount;
    return true;
}

int EventScheduleCatalog::GetServerSecondOfDay(uint64_t nowMs) const
{
    const auto elapsedSeconds = nowMs >= m_receivedAtMs ? static_cast<int64_t>((nowMs - m_receivedAtMs) / 1000) : 0;
    return static_cast<int>((m_serverSecondOfDay + elapsedSeconds) % SecondsPerDay);
}

ScheduledEventState EventScheduleCatalog::GetState(const ScheduledEvent& scheduledEvent, int serverSecondOfDay)
{
    ScheduledEventState state;
    if (scheduledEvent.StartMinutes.empty())
    {
        return state;
    }

    const auto durationSeconds = scheduledEvent.DurationMinutes * 60;
    auto nextStartIn = SecondsPerDay;
    for (const auto minute : scheduledEvent.StartMinutes)
    {
        const auto startSecond = minute * 60;

        // How long ago the event started at this time, also if that was yesterday.
        const auto sinceStart = (serverSecondOfDay - startSecond + SecondsPerDay) % SecondsPerDay;
        if (sinceStart < durationSeconds)
        {
            state.IsRunning = true;
            state.SecondsLeft = durationSeconds - sinceStart;
            state.StartMinute = minute;
            return state;
        }

        const auto untilStart = (startSecond - serverSecondOfDay + SecondsPerDay) % SecondsPerDay;
        if (untilStart > 0 && untilStart < nextStartIn)
        {
            nextStartIn = untilStart;
            state.StartMinute = minute;
        }
    }

    state.SecondsLeft = nextStartIn;
    return state;
}
} // namespace GameLogic::Events
