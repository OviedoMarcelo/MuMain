// Aims an automated cast at the caster rather than at the mouse.
#pragma once

class CHARACTER;

namespace GameLogic::Automation
{
// For the lifetime of the object, every target lookup the cast makes
// (GameLogic::Combat::CheckTarget and the class attack code that reads the
// selection) resolves to the caster: the selection under the cursor is set
// aside and the ground under the cursor is replaced by the caster's position.
// Without it a self or untargeted skill cast by the helper aims at, and when
// a wall is in the way walks towards, wherever the mouse rests. Both are put
// back when the object goes out of scope.
class ScopedCasterAim
{
public:
    explicit ScopedCasterAim(const CHARACTER* caster);
    ~ScopedCasterAim();

    ScopedCasterAim(const ScopedCasterAim&) = delete;
    ScopedCasterAim& operator=(const ScopedCasterAim&) = delete;

private:
    int m_previousSelection;
};
} // namespace GameLogic::Automation
