#pragma once

#include "GameLogic/Monsters/BossHealth.h"

#include <optional>

namespace GameLogic::Monsters
{
// The boss whose health the bar shows, if any (see BossHealth).
class BossHealthState
{
public:
    static BossHealthState& Instance();

    // Forgets the boss, e.g. when we connect to another server.
    void Reset();

    // Shows the health of the message, or hides the bar of its boss.
    void Apply(const BossHealth& health);

    // The boss whose health is shown, or nullptr if the bar is hidden.
    const BossHealth* Current() const;

private:
    std::optional<BossHealth> m_current;
};

inline BossHealthState& BossHealthBar()
{
    return BossHealthState::Instance();
}
} // namespace GameLogic::Monsters
