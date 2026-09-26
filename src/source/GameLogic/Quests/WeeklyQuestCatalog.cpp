#include "GameLogic/Quests/WeeklyQuestCatalog.h"

#include "Data/Translation/MultiLanguage.h"

#include <algorithm>

namespace GameLogic::Quests
{
namespace
{
// Offsets of the WeeklyQuestEntry message (C2 header with sub code).
constexpr int32_t PacketSize = 520;
constexpr int32_t IndexOffset = 5;
constexpr int32_t CountOffset = 6;
constexpr int32_t IsUpdateOffset = 7;
constexpr int32_t IsCompletedOffset = 8;
constexpr int32_t IsRewardedOffset = 9;
constexpr int32_t CurrentCountOffset = 12;
constexpr int32_t RequiredCountOffset = 16;
constexpr int32_t SecondsUntilResetOffset = 20;
constexpr int32_t IdOffset = 24;
constexpr int32_t IdLength = 64;
constexpr int32_t NameOffset = 88;
constexpr int32_t NameLength = 48;
constexpr int32_t DescriptionOffset = 136;
constexpr int32_t DescriptionLength = 256;
constexpr int32_t RewardsOffset = 392;
constexpr int32_t RewardsLength = 128;

// The strings are UTF-8 and padded with zeros.
std::wstring ReadString(const BYTE* data, int32_t offset, int32_t length)
{
    std::vector<wchar_t> buffer(static_cast<size_t>(length) + 1, L'\0');
    CMultiLanguage::ConvertFromUtf8(buffer.data(), reinterpret_cast<const char*>(data + offset), length);
    buffer[length] = L'\0';
    return std::wstring(buffer.data());
}

uint32_t ReadUInt32(const BYTE* data, int32_t offset)
{
    return static_cast<uint32_t>(data[offset]) | (static_cast<uint32_t>(data[offset + 1]) << 8) |
           (static_cast<uint32_t>(data[offset + 2]) << 16) | (static_cast<uint32_t>(data[offset + 3]) << 24);
}

WeeklyQuest ReadQuest(const BYTE* data)
{
    WeeklyQuest quest;
    quest.Id = ReadString(data, IdOffset, IdLength);
    quest.Name = ReadString(data, NameOffset, NameLength);
    quest.Description = ReadString(data, DescriptionOffset, DescriptionLength);
    quest.Rewards = ReadString(data, RewardsOffset, RewardsLength);
    quest.CurrentCount = ReadUInt32(data, CurrentCountOffset);
    quest.RequiredCount = ReadUInt32(data, RequiredCountOffset);
    quest.IsCompleted = data[IsCompletedOffset] != 0;
    quest.IsRewarded = data[IsRewardedOffset] != 0;
    return quest;
}
} // namespace

WeeklyQuestCatalog& WeeklyQuestCatalog::Instance()
{
    static WeeklyQuestCatalog instance;
    return instance;
}

std::chrono::seconds WeeklyQuestCatalog::GetTimeUntilReset() const
{
    const auto remaining = std::chrono::duration_cast<std::chrono::seconds>(m_resetTime - Clock::now());

    // No std::max: windows.h defines a max macro in this translation unit.
    return remaining < std::chrono::seconds::zero() ? std::chrono::seconds::zero() : remaining;
}

void WeeklyQuestCatalog::Reset()
{
    m_quests.clear();
    m_expectedCount = 0;
    m_isComplete = false;
    m_resetTime = {};
    ++m_revision;
}

bool WeeklyQuestCatalog::AddFromPacket(const BYTE* data, int32_t size)
{
    if (data == nullptr || size < PacketSize)
    {
        return false;
    }

    const auto index = data[IndexOffset];
    const auto count = data[CountOffset];
    const bool isUpdate = data[IsUpdateOffset] != 0;
    m_resetTime = Clock::now() + std::chrono::seconds(ReadUInt32(data, SecondsUntilResetOffset));

    // A list without quests is one message without a quest in it.
    if (!isUpdate && count == 0)
    {
        m_quests.clear();
        m_expectedCount = 0;
        m_isComplete = true;
        ++m_revision;
        return true;
    }

    auto quest = ReadQuest(data);
    if (quest.Id.empty())
    {
        return false;
    }

    const bool isAccepted = isUpdate ? Update(std::move(quest)) : AddToList(std::move(quest), index, count);
    if (isAccepted)
    {
        ++m_revision;
    }

    return isAccepted;
}

bool WeeklyQuestCatalog::AddToList(WeeklyQuest quest, BYTE index, BYTE count)
{
    if (index >= count)
    {
        return false;
    }

    // The first message of a list starts a new one, so that a second list
    // doesn't append to the quests we already know.
    if (index == 0)
    {
        m_quests.clear();
        m_expectedCount = count;
        m_isComplete = false;
    }
    else if (m_isComplete || count != m_expectedCount || index != m_quests.size())
    {
        return false;
    }

    m_quests.push_back(std::move(quest));
    m_isComplete = m_quests.size() >= count;
    return true;
}

bool WeeklyQuestCatalog::Update(WeeklyQuest quest)
{
    const auto known = std::find_if(m_quests.begin(), m_quests.end(),
                                    [&quest](const WeeklyQuest& candidate) { return candidate.Id == quest.Id; });
    if (known == m_quests.end())
    {
        return false;
    }

    *known = std::move(quest);
    return true;
}
} // namespace GameLogic::Quests
