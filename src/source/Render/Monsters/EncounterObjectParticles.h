#pragma once

class OBJECT;
class BMD;

// The particles around the objects of the boss encounters (the crystals, rune
// pillars and obelisks of GameLogic::Monsters::Custom): little flames which
// rise from the fire ones, blue wisps and sparkles from the ice ones. They come
// out of the floating crystal and the runes, and stop when the object breaks.
namespace Render::Monsters::EncounterObjectParticles
{
// Creates the particles of this frame. Returns false when the object isn't one of them.
bool Create(OBJECT* o, BMD* b);
} // namespace Render::Monsters::EncounterObjectParticles
