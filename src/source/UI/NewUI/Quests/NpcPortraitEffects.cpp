#include "UI/NewUI/Quests/NpcPortraitEffects.h"

#include <array>
#include <utility>

namespace UI::Quests::NpcPortraitEffects
{
namespace
{
// The numbers are the ones of the NPC definitions, like the names of the portraits.
constexpr uint16_t ElfLala = 242;

constexpr std::array<std::pair<uint16_t, Effect>, 1> EffectsByNpc = {{
    {ElfLala, Effect::GoldenMotes},
}};
} // namespace

Effect ForNpc(uint16_t npcNumber)
{
    for (const auto& [number, effect] : EffectsByNpc)
    {
        if (number == npcNumber)
        {
            return effect;
        }
    }

    return Effect::None;
}
} // namespace UI::Quests::NpcPortraitEffects
