// The parts of NpcQuestDialogState which talk to the server or look at the
// world. They are apart, so that the tests can use the rest without them.
#include "stdafx.h"

#include "GameLogic/Quests/NpcQuestDialogState.h"

#include "Engine/Object/ZzzCharacter.h"
#include "Network/Server/WSclient.h"

#include <cmath>

namespace GameLogic::Quests
{
namespace
{
// The server ignores an accepted dialog when the player is farther away than this, in tiles.
constexpr float MaximumNpcDistanceInTiles = 10.0f;

template <size_t Size>
void Send(const std::array<BYTE, Size>& packet)
{
    if (SocketClient == nullptr)
    {
        return;
    }

    SocketClient->Send(packet.data(), static_cast<int32_t>(packet.size()));
}
} // namespace

void NpcQuestDialogState::AnnounceSupport()
{
    Send(BuildClientFeaturesPacket());
}

void NpcQuestDialogState::Answer(bool accepted)
{
    if (!m_opened)
    {
        return;
    }

    Send(BuildAnswerPacket(m_opened->Id, accepted));
    Close();
}

bool NpcQuestDialogState::IsNpcInReach() const
{
    if (!m_opened || Hero == nullptr)
    {
        return false;
    }

    const int index = FindCharacterIndex(m_opened->NpcKey);
    if (index == MAX_CHARACTERS_CLIENT)
    {
        return false;
    }

    const auto& npcPosition = CharactersClient[index].Object.Position;
    const auto& heroPosition = Hero->Object.Position;
    const float distance = std::hypot(npcPosition[0] - heroPosition[0], npcPosition[1] - heroPosition[1]);
    return distance <= MaximumNpcDistanceInTiles * TERRAIN_SCALE;
}
} // namespace GameLogic::Quests
