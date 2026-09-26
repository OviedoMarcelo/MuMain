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

// Offsets of the QuestDetails message (C2 header with sub code), which is
// followed by one QuestObjective structure per objective.
constexpr int32_t DetailsCategoryOffset = 5;
constexpr int32_t DetailsPeriodOffset = 6;
constexpr int32_t DetailsCurrentStepOffset = 7;
constexpr int32_t DetailsObjectiveCountOffset = 8;
constexpr int32_t DetailsIsSequentialOffset = 9;
constexpr int32_t DetailsSecondsUntilResetOffset = 12;
constexpr int32_t DetailsIdOffset = 16;
constexpr int32_t DetailsObjectivesOffset = 80;
constexpr int32_t ObjectiveSize = 76;
constexpr int32_t ObjectiveCurrentCountOffset = 0;
constexpr int32_t ObjectiveRequiredCountOffset = 4;
constexpr int32_t ObjectiveIsDoneOffset = 8;
constexpr int32_t ObjectiveTextOffset = 12;
constexpr int32_t ObjectiveTextLength = 64;
constexpr BYTE HighestCategory = static_cast<BYTE>(QuestCategory::Zone);
constexpr BYTE HighestPeriod = static_cast<BYTE>(QuestPeriod::Once);

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

QuestObjective ReadObjective(const BYTE* data, int32_t offset)
{
    QuestObjective objective;
    objective.CurrentCount = ReadUInt32(data, offset + ObjectiveCurrentCountOffset);
    objective.RequiredCount = ReadUInt32(data, offset + ObjectiveRequiredCountOffset);
    objective.IsDone = data[offset + ObjectiveIsDoneOffset] != 0;
    objective.Text = ReadString(data, offset + ObjectiveTextOffset, ObjectiveTextLength);
    return objective;
}

// An unknown category or period comes from a newer server; such a quest is shown as a weekly one.
QuestCategory ReadCategory(BYTE value)
{
    return value <= HighestCategory ? static_cast<QuestCategory>(value) : QuestCategory::Weekly;
}

QuestPeriod ReadPeriod(BYTE value)
{
    return value <= HighestPeriod ? static_cast<QuestPeriod>(value) : QuestPeriod::Weekly;
}

void ReadDetails(const BYTE* data, WeeklyQuest& quest)
{
    const auto objectiveCount = data[DetailsObjectiveCountOffset];
    quest.HasDetails = true;
    quest.Category = ReadCategory(data[DetailsCategoryOffset]);
    quest.Period = ReadPeriod(data[DetailsPeriodOffset]);
    quest.IsSequential = data[DetailsIsSequentialOffset] != 0;
    quest.CurrentStep = data[DetailsCurrentStepOffset];
    quest.ResetTime = WeeklyQuest::Clock::now() + std::chrono::seconds(ReadUInt32(data, DetailsSecondsUntilResetOffset));
    quest.Objectives.clear();
    quest.Objectives.reserve(objectiveCount);
    for (int32_t i = 0; i < objectiveCount; ++i)
    {
        quest.Objectives.push_back(ReadObjective(data, DetailsObjectivesOffset + i * ObjectiveSize));
    }
}

// An update of a quest only carries its progress, its details follow in their own message.
// Until then, the quest keeps the details which we knew.
void KeepDetails(const WeeklyQuest& known, WeeklyQuest& update)
{
    update.HasDetails = known.HasDetails;
    update.Category = known.Category;
    update.Period = known.Period;
    update.IsSequential = known.IsSequential;
    update.CurrentStep = known.CurrentStep;
    update.Objectives = known.Objectives;
    update.ResetTime = known.ResetTime;
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

std::chrono::seconds WeeklyQuestCatalog::GetTimeUntilReset(const WeeklyQuest& quest)
{
    const auto remaining = std::chrono::duration_cast<std::chrono::seconds>(quest.ResetTime - Clock::now());
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
    auto* known = Find(quest.Id);
    if (known == nullptr)
    {
        return false;
    }

    KeepDetails(*known, quest);
    *known = std::move(quest);
    return true;
}

bool WeeklyQuestCatalog::AddDetailsFromPacket(const BYTE* data, int32_t size)
{
    if (data == nullptr || size < DetailsObjectivesOffset)
    {
        return false;
    }

    const auto objectiveCount = data[DetailsObjectiveCountOffset];
    if (size < DetailsObjectivesOffset + objectiveCount * ObjectiveSize)
    {
        return false;
    }

    auto* quest = Find(ReadString(data, DetailsIdOffset, IdLength));
    if (quest == nullptr)
    {
        return false;
    }

    ReadDetails(data, *quest);
    ++m_revision;
    return true;
}

WeeklyQuest* WeeklyQuestCatalog::Find(const std::wstring& id)
{
    const auto known = std::find_if(m_quests.begin(), m_quests.end(),
                                    [&id](const WeeklyQuest& candidate) { return candidate.Id == id; });
    return known == m_quests.end() ? nullptr : &*known;
}
} // namespace GameLogic::Quests
