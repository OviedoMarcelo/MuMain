#include "stdafx.h"

#include "Render/Monsters/MonsterGlow.h"

#include "Engine/Object/ZzzObject.h"
#include "GameLogic/Monsters/MonsterAppearance.h"
#include "Render/Models/ZzzBMD.h"
#include "UI/NewUI/NewUISystem.h"

namespace Render::Monsters::Glow
{
namespace
{
using GameLogic::Monsters::MonsterGlow;

constexpr float Bright = 1.f;

// The glow level option which items need to glow like these levels.
constexpr int RenderLevelForLevel7 = 1;
constexpr int RenderLevelForLevel9 = 2;
constexpr int RenderLevelForLevel11 = 3;
constexpr int RenderLevelForLevel13 = 4;

// The body light of the golden invasion monsters.
constexpr float GoldenRed = 1.f;
constexpr float GoldenGreen = 0.6f;
constexpr float GoldenBlue = 0.3f;

int GetRequiredRenderLevel(MonsterGlow glow)
{
    switch (glow)
    {
    case MonsterGlow::Level7:
        return RenderLevelForLevel7;
    case MonsterGlow::Level9:
        return RenderLevelForLevel9;
    case MonsterGlow::Level11:
        return RenderLevelForLevel11;
    default:
        return RenderLevelForLevel13;
    }
}

void RenderColor(BMD* b, OBJECT* o, int renderType)
{
    RenderPartObjectBodyColor(b, o, o->Type, o->Alpha, renderType | RENDER_BRIGHT, Bright);
}

// The shine of the levels +11 and up, drawn first like for items.
void RenderShine(BMD* b, OBJECT* o, int chromeType)
{
    RenderPartObjectBodyColor2(b, o, o->Type, o->Alpha, chromeType | RENDER_BRIGHT, Bright);
}

void RenderGolden(BMD* b, OBJECT* o)
{
    vec3_t backupBodyLight;
    VectorCopy(b->BodyLight, backupBodyLight);
    Vector(GoldenRed, GoldenGreen, GoldenBlue, b->BodyLight);
    RenderColor(b, o, RENDER_METAL);
    RenderColor(b, o, RENDER_CHROME);
    VectorCopy(backupBodyLight, b->BodyLight);
}

void RenderLevel(BMD* b, OBJECT* o, MonsterGlow glow)
{
    if (g_pOption->GetRenderLevel() < GetRequiredRenderLevel(glow))
    {
        return;
    }

    if (glow == MonsterGlow::Level11)
    {
        RenderShine(b, o, RENDER_CHROME2);
    }
    else if (glow == MonsterGlow::Level13 || glow == MonsterGlow::Level15)
    {
        RenderShine(b, o, RENDER_CHROME4);
    }

    if (glow != MonsterGlow::Level7)
    {
        RenderColor(b, o, RENDER_METAL);
    }

    RenderColor(b, o, RENDER_CHROME);
}
} // namespace

void Render(const CHARACTER* c, OBJECT* o)
{
    const auto glow = static_cast<MonsterGlow>(c->AppearanceGlow);
    if (glow == MonsterGlow::None)
    {
        return;
    }

    BMD* b = &Models[o->Type];
    if (glow == MonsterGlow::Golden)
    {
        RenderGolden(b, o);
        return;
    }

    RenderLevel(b, o, glow);
}
} // namespace Render::Monsters::Glow
