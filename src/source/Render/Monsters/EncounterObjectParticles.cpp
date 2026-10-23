#include "stdafx.h"

#include "Render/Monsters/EncounterObjectParticles.h"

#include "Engine/Object/ZzzObject.h"
#include "Render/Effects/ZzzEffect.h"
#include "Render/Models/ZzzBMD.h"
#include "Render/Textures/ZzzOpenglUtil.h"

#include <array>
#include <cmath>

namespace Render::Monsters::EncounterObjectParticles
{
namespace
{
// The bones of the models of tools/models/make_encounter_objects.js.
constexpr int RootBone = 0;
constexpr int FloatingBone = 1;

// A small flame which rises slowly; BITMAP_TRUE_FIRE is orange, BITMAP_TRUE_BLUE blue.
constexpr int FlameSubType = 8;
constexpr float FlameScale = 0.7f;
// A sparkle which twinkles where it appears.
constexpr int SparkleSubType = 5;
constexpr float SparkleScale = 0.6f;
// The sparkles are light blue; the flames keep the color of their texture.
constexpr float SparkleRed = 0.6f;
constexpr float SparkleGreen = 0.85f;
constexpr float SparkleBlue = 1.f;
constexpr float FullLight = 1.f;

// A particle comes out of an emitter on average once in this many frames (at 25 frames per second).
constexpr int FlameFrames = 2;
constexpr int SparkleFrames = 4;

constexpr float DegreesToRadians = 3.14159265f / 180.f;
constexpr int FullCircleDegrees = 360;

enum class Element
{
    Fire,
    Ice,
};

// Where particles come out: around the axis of a bone, at a distance and between two heights.
struct Emitter
{
    int Bone;
    float Radius;
    float Bottom;
    float Top;
};

constexpr Emitter NoEmitter = {RootBone, 0.f, 0.f, 0.f};

struct ObjectParticles
{
    int ModelType;
    Element Kind;
    Emitter Crystal;
    // The runes, if the object has any.
    Emitter Runes;
};

// The shapes of the models: the floating crystals are around the floating bone, the runes of the pillar
// are a band around its shaft, the ones of the obelisk strips on its faces.
constexpr Emitter CrystalCluster = {FloatingBone, 45.f, -60.f, 60.f};
constexpr Emitter PillarCrown = {FloatingBone, 26.f, -40.f, 40.f};
constexpr Emitter PillarRunes = {RootBone, 46.f, 150.f, 250.f};
constexpr Emitter ObeliskCrown = {FloatingBone, 20.f, -40.f, 40.f};
constexpr Emitter ObeliskRunes = {RootBone, 34.f, 110.f, 400.f};

constexpr std::array<ObjectParticles, 6> Objects = {{
    {MODEL_ICE_CRYSTAL, Element::Ice, CrystalCluster, NoEmitter},
    {MODEL_FIRE_CRYSTAL, Element::Fire, CrystalCluster, NoEmitter},
    {MODEL_ICE_RUNE_PILLAR, Element::Ice, PillarCrown, PillarRunes},
    {MODEL_FIRE_RUNE_PILLAR, Element::Fire, PillarCrown, PillarRunes},
    {MODEL_ICE_OBELISK, Element::Ice, ObeliskCrown, ObeliskRunes},
    {MODEL_FIRE_OBELISK, Element::Fire, ObeliskCrown, ObeliskRunes},
}};

const ObjectParticles* Find(int modelType)
{
    for (const auto& object : Objects)
    {
        if (object.ModelType == modelType)
        {
            return &object;
        }
    }

    return nullptr;
}

float RandomBetween(float minimum, float maximum)
{
    constexpr int Steps = 1000;
    return minimum + (maximum - minimum) * static_cast<float>(rand() % Steps) / Steps;
}

// A random point of the emitter, in the world.
void GetRandomPoint(OBJECT* o, BMD* b, const Emitter& emitter, vec3_t position)
{
    const float angle = static_cast<float>(rand() % FullCircleDegrees) * DegreesToRadians;
    vec3_t local;
    Vector(emitter.Radius * std::cos(angle), emitter.Radius * std::sin(angle), RandomBetween(emitter.Bottom, emitter.Top),
           local);
    b->TransformPosition(o->BoneTransform[emitter.Bone], local, position, true);
}

void CreateFlame(OBJECT* o, BMD* b, const Emitter& emitter, Element kind)
{
    vec3_t position;
    GetRandomPoint(o, b, emitter, position);
    vec3_t light;
    Vector(FullLight, FullLight, FullLight, light);
    const int texture = kind == Element::Fire ? BITMAP_TRUE_FIRE : BITMAP_TRUE_BLUE;
    CreateParticle(texture, position, o->Angle, light, FlameSubType, FlameScale * o->Scale);
}

void CreateSparkle(OBJECT* o, BMD* b, const Emitter& emitter)
{
    vec3_t position;
    GetRandomPoint(o, b, emitter, position);
    vec3_t light;
    Vector(SparkleRed, SparkleGreen, SparkleBlue, light);
    CreateParticle(BITMAP_SHINY, position, o->Angle, light, SparkleSubType, SparkleScale * o->Scale);
}

void CreateFrom(OBJECT* o, BMD* b, const Emitter& emitter, Element kind)
{
    if (emitter.Radius <= 0.f)
    {
        return;
    }

    if (rand_fps_check(FlameFrames))
    {
        CreateFlame(o, b, emitter, kind);
    }

    if (kind == Element::Ice && rand_fps_check(SparkleFrames))
    {
        CreateSparkle(o, b, emitter);
    }
}
} // namespace

bool Create(OBJECT* o, BMD* b)
{
    const ObjectParticles* object = Find(o->Type);
    if (object == nullptr)
    {
        return false;
    }

    // A broken object sinks into the ground without them.
    if (o->CurrentAction == MONSTER01_DIE)
    {
        return true;
    }

    CreateFrom(o, b, object->Crystal, object->Kind);
    CreateFrom(o, b, object->Runes, object->Kind);
    return true;
}
} // namespace Render::Monsters::EncounterObjectParticles
