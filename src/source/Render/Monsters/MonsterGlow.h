#pragma once

class CHARACTER;
class OBJECT;

// The glow of a monster which looks different than the others of its kind
// (see GameLogic::Monsters::MonsterAppearance), e.g. the boss of an encounter.
namespace Render::Monsters::Glow
{
// Draws the glow over the body of the monster, which was drawn before. Like
// the glow of items, the levels +7 to +15 follow the option of the glow level;
// the golden glow of the golden invasion monsters is always drawn.
void Render(const CHARACTER* c, OBJECT* o);
} // namespace Render::Monsters::Glow
