#include "stdafx.h"

#include "GameLogic/Monsters/MonsterAppearanceApplier.h"

#include "Engine/Object/ZzzCharacter.h"

#include <algorithm>

namespace GameLogic::Monsters
{
namespace
{
// A huge or tiny monster would cover the screen or vanish, and couldn't be clicked anymore.
constexpr int MinimumScalePercent = 10;
constexpr int MaximumScalePercent = 1000;
constexpr float PercentFactor = 0.01f;

void ApplyScale(CHARACTER* c, int scalePercent)
{
    const int clamped = std::clamp(scalePercent, MinimumScalePercent, MaximumScalePercent);

    // The scale of the monster was set when it was created, so it's always the one of its kind.
    c->Object.Scale *= static_cast<float>(clamped) * PercentFactor;
}

void ApplyName(CHARACTER* c, const std::wstring& name)
{
    if (name.empty())
    {
        return;
    }

    wcsncpy_s(c->ID, MAX_MONSTER_NAME + 1, name.c_str(), _TRUNCATE);
}
} // namespace

bool ApplyMonsterAppearance(const MonsterAppearance& appearance)
{
    CHARACTER* c = FindCharacterByKey(appearance.Key);
    if (c == nullptr || c == Hero)
    {
        return false;
    }

    ApplyScale(c, appearance.ScalePercent);
    ApplyName(c, appearance.Name);
    c->AppearanceGlow = static_cast<BYTE>(appearance.Glow);
    return true;
}
} // namespace GameLogic::Monsters
