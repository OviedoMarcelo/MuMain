#include "Core/Platform/WinCompat.h"
#include "Data/Translation/MultiLanguage.h"
#include "GameLogic/Quests/WeeklyQuestCatalog.h"
#include "UI/Quests/QuestList.h"

#include "doctest.h"

#include <algorithm>
#include <string>
#include <vector>

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
using GameLogic::Quests::WeeklyQuests;

constexpr size_t PacketSize = 520;
constexpr size_t IndexOffset = 5;
constexpr size_t CountOffset = 6;
constexpr size_t IsUpdateOffset = 7;
constexpr size_t IsCompletedOffset = 8;
constexpr size_t CurrentCountOffset = 12;
constexpr size_t RequiredCountOffset = 16;
constexpr size_t SecondsUntilResetOffset = 20;
constexpr size_t IdOffset = 24;
constexpr size_t NameOffset = 88;

void WriteUInt32(std::vector<BYTE>& packet, size_t offset, uint32_t value)
{
    for (size_t i = 0; i < 4; ++i)
    {
        packet[offset + i] = static_cast<BYTE>(value >> (8 * i));
    }
}

void WriteString(std::vector<BYTE>& packet, size_t offset, const std::string& text)
{
    std::copy(text.begin(), text.end(), packet.begin() + static_cast<std::ptrdiff_t>(offset));
}

std::vector<BYTE> MakePacket(BYTE index, BYTE count, const std::string& id, uint32_t currentCount = 0,
                             bool isUpdate = false)
{
    std::vector<BYTE> packet(PacketSize);
    packet[IndexOffset] = index;
    packet[CountOffset] = count;
    packet[IsUpdateOffset] = isUpdate ? 1 : 0;
    WriteUInt32(packet, CurrentCountOffset, currentCount);
    WriteUInt32(packet, RequiredCountOffset, 10);
    WriteUInt32(packet, SecondsUntilResetOffset, 3600);
    WriteString(packet, IdOffset, id);
    WriteString(packet, NameOffset, "Quest " + id);
    return packet;
}

bool Add(const std::vector<BYTE>& packet)
{
    return WeeklyQuests().AddFromPacket(packet.data(), static_cast<int32_t>(packet.size()));
}

using Category = GameLogic::Quests::QuestCategory;

constexpr size_t DetailsCategoryOffset = 5;
constexpr size_t DetailsPeriodOffset = 6;
constexpr size_t DetailsCurrentStepOffset = 7;
constexpr size_t DetailsObjectiveCountOffset = 8;
constexpr size_t DetailsIsSequentialOffset = 9;
constexpr size_t DetailsIdOffset = 16;
constexpr size_t DetailsObjectivesOffset = 80;
constexpr size_t ObjectiveSize = 76;
constexpr BYTE OncePeriod = 2;

struct TestObjective
{
    std::string Text;
    uint32_t CurrentCount;
    uint32_t RequiredCount;
    bool IsDone;
};

std::vector<BYTE> MakeDetailsPacket(const std::string& id, Category category, const std::vector<TestObjective>& objectives)
{
    std::vector<BYTE> packet(DetailsObjectivesOffset + objectives.size() * ObjectiveSize);
    packet[DetailsCategoryOffset] = static_cast<BYTE>(category);
    packet[DetailsPeriodOffset] = OncePeriod;
    packet[DetailsObjectiveCountOffset] = static_cast<BYTE>(objectives.size());
    packet[DetailsIsSequentialOffset] = 1;
    WriteString(packet, DetailsIdOffset, id);

    BYTE currentStep = static_cast<BYTE>(objectives.size());
    for (size_t i = 0; i < objectives.size(); ++i)
    {
        const auto offset = DetailsObjectivesOffset + i * ObjectiveSize;
        WriteUInt32(packet, offset, objectives[i].CurrentCount);
        WriteUInt32(packet, offset + 4, objectives[i].RequiredCount);
        packet[offset + 8] = objectives[i].IsDone ? 1 : 0;
        WriteString(packet, offset + 12, objectives[i].Text);
        if (!objectives[i].IsDone && currentStep == objectives.size())
        {
            currentStep = static_cast<BYTE>(i);
        }
    }

    packet[DetailsCurrentStepOffset] = currentStep;
    return packet;
}

bool AddDetails(const std::vector<BYTE>& packet)
{
    return WeeklyQuests().AddDetailsFromPacket(packet.data(), static_cast<int32_t>(packet.size()));
}
} // namespace

TEST_CASE("weekly quest catalog accepts a complete ordered list")
{
    WeeklyQuests().Reset();

    CHECK(Add(MakePacket(0, 2, "a", 3)));
    CHECK_FALSE(WeeklyQuests().IsAvailable());
    CHECK(Add(MakePacket(1, 2, "b")));
    CHECK(WeeklyQuests().IsAvailable());

    const auto& quests = WeeklyQuests().GetQuests();
    REQUIRE(quests.size() == 2);
    CHECK(quests[0].Id == L"a");
    CHECK(quests[0].Name == L"Quest a");
    CHECK(quests[0].CurrentCount == 3);
    CHECK(quests[0].RequiredCount == 10);
    CHECK(WeeklyQuests().GetTimeUntilReset().count() > 3500);
}

TEST_CASE("weekly quest catalog rejects an out-of-order list")
{
    WeeklyQuests().Reset();

    CHECK_FALSE(Add(MakePacket(1, 2, "b")));
    CHECK(WeeklyQuests().GetQuests().empty());
    CHECK_FALSE(WeeklyQuests().IsAvailable());
}

TEST_CASE("weekly quest catalog accepts an empty list")
{
    WeeklyQuests().Reset();
    CHECK(Add(MakePacket(0, 1, "a")));

    CHECK(Add(MakePacket(0, 0, "")));
    CHECK(WeeklyQuests().IsAvailable());
    CHECK(WeeklyQuests().GetQuests().empty());
}

TEST_CASE("weekly quest catalog updates a known quest")
{
    WeeklyQuests().Reset();
    CHECK(Add(MakePacket(0, 1, "a", 3)));
    const auto revision = WeeklyQuests().GetRevision();

    auto update = MakePacket(0, 1, "a", 10, true);
    update[IsCompletedOffset] = 1;
    CHECK(Add(update));

    const auto& quest = WeeklyQuests().GetQuests().front();
    CHECK(quest.CurrentCount == 10);
    CHECK(quest.IsCompleted);
    CHECK(quest.IsRewardPending());
    CHECK(WeeklyQuests().GetRevision() != revision);
}

TEST_CASE("weekly quest catalog ignores an update of an unknown quest")
{
    WeeklyQuests().Reset();
    CHECK(Add(MakePacket(0, 1, "a", 3)));

    CHECK_FALSE(Add(MakePacket(0, 1, "b", 5, true)));
    CHECK(WeeklyQuests().GetQuests().size() == 1);
    CHECK(WeeklyQuests().GetQuests().front().CurrentCount == 3);
}

TEST_CASE("weekly quest catalog rejects a message which is too short")
{
    WeeklyQuests().Reset();
    const auto packet = MakePacket(0, 1, "a");

    CHECK_FALSE(WeeklyQuests().AddFromPacket(packet.data(), static_cast<int32_t>(PacketSize - 1)));
    CHECK(WeeklyQuests().GetQuests().empty());
}

TEST_CASE("quest details add the category and the steps of a known quest")
{
    WeeklyQuests().Reset();
    CHECK(Add(MakePacket(0, 1, "main-1")));
    const auto revision = WeeklyQuests().GetRevision();

    const auto details = MakeDetailsPacket("main-1", Category::Main, {{"Talk to the Guardian", 1, 1, true},
                                                                      {"Kill 50 Skeletons", 12, 50, false}});
    CHECK(AddDetails(details));

    const auto& quest = WeeklyQuests().GetQuests().front();
    CHECK(quest.Category == Category::Main);
    CHECK(quest.Period == GameLogic::Quests::QuestPeriod::Once);
    CHECK(quest.IsSequential);
    CHECK(quest.CurrentStep == 1);
    CHECK(quest.HasSteps());
    REQUIRE(quest.Objectives.size() == 2);
    CHECK(quest.Objectives[0].Text == L"Talk to the Guardian");
    CHECK(quest.Objectives[0].IsDone);
    CHECK(quest.Objectives[1].CurrentCount == 12);
    CHECK(quest.Objectives[1].RequiredCount == 50);
    CHECK(WeeklyQuests().GetRevision() != revision);
}

TEST_CASE("quest details of an unknown quest or with missing steps are rejected")
{
    WeeklyQuests().Reset();
    CHECK(Add(MakePacket(0, 1, "a")));

    CHECK_FALSE(AddDetails(MakeDetailsPacket("b", Category::Daily, {})));

    auto truncated = MakeDetailsPacket("a", Category::Daily, {{"Step", 0, 1, false}});
    truncated.resize(truncated.size() - 1);
    CHECK_FALSE(AddDetails(truncated));
    CHECK(WeeklyQuests().GetQuests().front().Objectives.empty());
}

TEST_CASE("an update of a quest keeps its details until the new ones arrive")
{
    WeeklyQuests().Reset();
    CHECK(Add(MakePacket(0, 1, "a")));
    CHECK(AddDetails(MakeDetailsPacket("a", Category::Zone, {{"Enter Devias", 0, 1, false}, {"Kill 10", 0, 10, false}})));

    CHECK(Add(MakePacket(0, 1, "a", 1, true)));

    const auto& quest = WeeklyQuests().GetQuests().front();
    CHECK(quest.CurrentCount == 1);
    CHECK(quest.Category == Category::Zone);
    CHECK(quest.Objectives.size() == 2);
}

TEST_CASE("quest list groups the quests by category with headings")
{
    std::vector<GameLogic::Quests::WeeklyQuest> quests(4);
    for (auto& quest : quests)
    {
        quest.HasDetails = true;
    }

    quests[0].Category = Category::Weekly;
    quests[1].Category = Category::Main;
    quests[2].Category = Category::Weekly;
    quests[3].Category = Category::Daily;

    const auto rows = UI::Quests::BuildListRows(quests);

    // Story, then daily, then weekly - each with a heading.
    REQUIRE(rows.size() == 7);
    CHECK(rows[0].IsHeading());
    CHECK(rows[0].Category == Category::Main);
    CHECK(rows[1].QuestIndex == 1);
    CHECK(rows[2].IsHeading());
    CHECK(rows[3].QuestIndex == 3);
    CHECK(rows[4].IsHeading());
    CHECK(rows[5].QuestIndex == 0);
    CHECK(rows[6].QuestIndex == 2);
}

TEST_CASE("quest list of an older server has no headings")
{
    std::vector<GameLogic::Quests::WeeklyQuest> quests(2);

    const auto rows = UI::Quests::BuildListRows(quests);

    REQUIRE(rows.size() == 2);
    CHECK_FALSE(rows[0].IsHeading());
    CHECK(rows[1].QuestIndex == 1);
    CHECK(UI::Quests::BuildListRows({}).empty());
}

TEST_CASE("quest list shows the heading even for a single category")
{
    std::vector<GameLogic::Quests::WeeklyQuest> quests(1);
    quests[0].HasDetails = true;

    const auto rows = UI::Quests::BuildListRows(quests);

    REQUIRE(rows.size() == 2);
    CHECK(rows[0].IsHeading());
    CHECK(rows[0].Category == Category::Weekly);
    CHECK(rows[1].QuestIndex == 0);
}

TEST_CASE("quest list shows the weekly reset, the daily one, or none when nothing resets")
{
    using GameLogic::Quests::QuestPeriod;
    std::vector<GameLogic::Quests::WeeklyQuest> quests(3);
    for (auto& quest : quests)
    {
        quest.HasDetails = true;
        quest.Period = QuestPeriod::Once;
    }

    CHECK(UI::Quests::FindQuestOfListReset(quests) == UI::Quests::QuestListRow::NoQuest);

    quests[2].Period = QuestPeriod::Daily;
    CHECK(UI::Quests::FindQuestOfListReset(quests) == 2);

    quests[1].Period = QuestPeriod::Weekly;
    CHECK(UI::Quests::FindQuestOfListReset(quests) == 1);

    // Without details, a quest is a weekly one.
    quests[0].HasDetails = false;
    CHECK(UI::Quests::FindQuestOfListReset(quests) == 0);
}

TEST_CASE("quest step line shows the counter only while it's open")
{
    GameLogic::Quests::QuestObjective objective;
    objective.Text = L"Kill 50 Skeletons";
    objective.CurrentCount = 12;
    objective.RequiredCount = 50;
    CHECK(UI::Quests::FormatObjectiveLine(objective) == L"[  ] Kill 50 Skeletons (12/50)");

    objective.IsDone = true;
    CHECK(UI::Quests::FormatObjectiveLine(objective) == L"[x] Kill 50 Skeletons");

    objective.IsDone = false;
    objective.RequiredCount = 1;
    CHECK(UI::Quests::FormatObjectiveLine(objective) == L"[  ] Kill 50 Skeletons");
}
