#pragma once

#include "Core/Platform/WinCompat.h"

#include <cstdint>
#include <string>
#include <vector>

// The events which start by themselves at fixed times of the day, e.g. Blood
// Castle or an invasion, as the server told us about them.
//
// The server sends one EventScheduleEntry message per event when we entered
// the game, and the whole list again when a timetable changes. Each message
// carries the time of the server, so that the next start doesn't depend on
// the clock of this computer or its time zone.
namespace GameLogic::Events
{
struct ScheduledEvent
{
    std::wstring Name;
    // How long the event lasts after each start.
    int DurationMinutes = 0;
    // The minutes since midnight in the time zone of the times, sorted.
    std::vector<int> StartMinutes;
};

// Where an event stands at a point in time.
struct ScheduledEventState
{
    bool IsRunning = false;
    // Until the running event ends, or until the next start.
    int SecondsLeft = 0;
    // The minute of the day of the next start, or of the start of the running event.
    int StartMinute = 0;
};

class EventScheduleCatalog
{
public:
    static constexpr int SecondsPerDay = 24 * 60 * 60;

    static EventScheduleCatalog& Instance();

    const std::vector<ScheduledEvent>& GetEvents() const
    {
        return m_isComplete ? m_events : m_emptyEvents;
    }

    // True when the server told us about its events, even if there are none.
    // Until then, the feature stays hidden - the server may be an older one
    // which doesn't send them.
    bool IsAvailable() const
    {
        return m_isComplete;
    }

    // Forgets everything, e.g. when the character changed.
    void Reset();

    // Takes one EventScheduleEntry message, received at the given tick count
    // (in milliseconds). Returns false when the data isn't plausible, so that
    // the caller can ignore it.
    bool AddFromPacket(const BYTE* data, int32_t size, uint64_t receivedAtMs);

    // The current time as seconds since midnight, in the time zone of the times.
    int GetServerSecondOfDay(uint64_t nowMs) const;

    // The name of the time zone of the times, e.g. "Argentina". It's empty when
    // the times are in the time zone of the server.
    const std::wstring& GetTimeZoneName() const
    {
        return m_timeZoneName;
    }

    // Where the event stands at the given time of the server.
    static ScheduledEventState GetState(const ScheduledEvent& scheduledEvent, int serverSecondOfDay);

private:
    std::vector<ScheduledEvent> m_events;
    const std::vector<ScheduledEvent> m_emptyEvents;
    size_t m_expectedCount = 0;
    bool m_isComplete = false;
    int m_serverSecondOfDay = 0;
    std::wstring m_timeZoneName;
    uint64_t m_receivedAtMs = 0;
};

inline EventScheduleCatalog& EventSchedule()
{
    return EventScheduleCatalog::Instance();
}
} // namespace GameLogic::Events
