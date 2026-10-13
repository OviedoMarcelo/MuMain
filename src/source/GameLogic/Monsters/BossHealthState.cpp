#include "GameLogic/Monsters/BossHealthState.h"

namespace GameLogic::Monsters
{
BossHealthState& BossHealthState::Instance()
{
    static BossHealthState instance;
    return instance;
}

void BossHealthState::Reset()
{
    this->m_current.reset();
}

void BossHealthState::Apply(const BossHealth& health)
{
    if (health.IsVisible)
    {
        this->m_current = health;
        return;
    }

    // Only the bar of this boss is hidden, as another boss may be shown meanwhile.
    if (this->m_current && this->m_current->Key == health.Key)
    {
        this->m_current.reset();
    }
}

const BossHealth* BossHealthState::Current() const
{
    return this->m_current ? &*this->m_current : nullptr;
}
} // namespace GameLogic::Monsters
