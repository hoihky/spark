# Particles

## Class Design: `ParticleEmitterComponent`

CPU-simulated billboard particles collected into `SceneRenderParams::particles`:

```cpp
class ParticleEmitterComponent final : public GameComponent {
public:
    void SetEmissionRate(float rate) noexcept;
    void SetLifetime(float minSec, float maxSec) noexcept;
    void SetStartEndSize(float start, float end) noexcept;
    void SetStartEndColor(const Vector4& start, const Vector4& end) noexcept;
    void SetGravity(const Vector3& g) noexcept;
    void SetEmissionDirection(const Vector3& dir) noexcept;
    void SetUseLocalEmission(bool local) noexcept;
    void Burst(GameObject& owner, std::uint32_t count);
    void CollectInstances(Array<SceneParticleInstance>& out) const;
};
```

Requires `TransformComponent` on the same entity (emission origin = world translation). With `SetUseLocalEmission(true)`, the emission direction follows entity rotation.

## Built-in presets (`VfxLibrary`)

See [`docs/VFX_ROADMAP.md`](../../VFX_ROADMAP.md). Apply fire, snow, smoke, sparkle, explosion, or impact:

```cpp
#include "spark/scene/vfx/VfxLibrary.hpp"

auto* emitter = fxGo->AddComponent<ParticleEmitterComponent>();
VfxLibrary::ApplyBuiltin(VfxBuiltinId::Fire, *emitter);

// One-shot burst (explosion / impact use emission rate 0)
VfxLibrary::ApplyBuiltin(VfxBuiltinId::Explosion, *emitter);
emitter->Burst(*fxGo, 96);
```

## `VfxPlayerComponent` + assets (P2)

Load a `.sparkvfx` asset or built-in by name:

```cpp
#include "spark/ecs/components/rendering/VfxPlayerComponent.hpp"
#include "spark/scene/vfx/VfxSubsystemProcess.hpp"

// Attached player
auto* player = fxGo->AddComponent<VfxPlayerComponent>();
player->SetVfxAssetKey("vfx/explosion");  // or "explosion"
player->PlayOnce(*fxGo);

// World-space one-shot (pooled; call ProcessVfx each frame via Game base)
world.GetVfxSubsystem().Queue("explosion", hitPosition);
```

`.sparkvfx` format (`sparkvfx_v1`):

```text
builtin explosion
burst 112
```

Composite (`sparkvfx_v1`):

```text
composite
phase 0.000 builtin rocket_trail duration 0.280
phase 0.280 builtin explosion burst 96
phase 0.420 builtin sparkle burst 48
prefab prefabs/fx_extra.sparkscene
```

Built-in composites: `fireworks`, `confetti`, `meteor_strike`, `magic_impact`, `smoke_grenade` (via `VfxEffectDefinition`).

`VfxLibrary` ships **30** single-emitter presets (weather, ambient, combat, RPG) plus the composites above. See `VfxShowcaseDetail.hpp` for the full showcase catalog.

## Animation events + scripting (P5)

Attach `AnimationEventVfxComponent` beside `AnimationEventReceiverComponent` to play VFX when markers fire:

```cpp
#include "spark/ecs/components/animation/AnimationEventVfxComponent.hpp"

auto* vfxEvents = actor->AddComponent<AnimationEventVfxComponent>();
vfxEvents->AddBinding("footstep", "impact");
```

C# / managed interop:

```csharp
Native.spark_vfx_play(world, "explosion", hitX, hitY, hitZ);
Native.spark_world_process_vfx(world);
```

## Curves + modules (P4)

Life curves replace linear start/end interpolation when set on the emitter:

```cpp
#include "spark/scene/vfx/ParticleCurves.hpp"
#include "spark/scene/vfx/modules/ParticleModuleRegistry.hpp"

emitter->SetEmissionModuleId("ring");  // continuous | burst_only | ring
emitter->SetRingRadius(0.6F);

ParticleFloatCurve sizeCurve{};
sizeCurve.SetEndpoints(0.2F, 0.02F);
emitter->SetSizeCurve(sizeCurve);
```

## Fire Emitter (manual tuning)

```cpp
auto* fxGo = world.CreateGameObject();
fxGo->AddComponent<TransformComponent>()->SetTranslation({0, 1, 0});

auto* emitter = fxGo->AddComponent<ParticleEmitterComponent>();
VfxLibrary::ApplyBuiltin(VfxBuiltinId::Fire, *emitter);
```

## Gem Sparkles (from `Maze3DDemo`)

Attach a subtle emitter to collectible props:

```cpp
if (ParticleEmitterComponent* pe = gem->AddComponent<ParticleEmitterComponent>()) {
    pe->SetMaxParticles(200);
    pe->SetEmissionRate(32.0F);
    pe->SetLifetime(0.4F, 1.05F);
    pe->SetStartEndSize(0.11F, 0.018F);
    pe->SetStartEndColor(gemColor, Vector4{gemColor.x, gemColor.y, gemColor.z, 0.0F});
}
```

## Enable in Submit

```cpp
SubmitStandardLitSceneFromWorldWithCamera(
    world, context, sunDir, sunColor, intensity, ambient,
    true,   // enableParticles
    sceneTime);
```

Or pass `enableParticles = true` to `SubmitStandardLitSceneFromWorld`.

## `SceneParticleInstance`

```cpp
struct SceneParticleInstance {
    Vector3 position{};
    float size = 0.1F;
    Vector4 color{1, 1, 1, 1};
    Vector4 uvRect{0, 0, 1, 1};
    std::int32_t textureLayer = -1;  // resolved at submit from ParticleEmitterComponent::GetTexture()
};
```

See `VfxShowcaseDemo` (SparkDemo **#3**) for the built-in library, live tuning, and saving custom `.sparkvfx` assets (`VfxShowcaseDetail.hpp`).

Part 3 complete → **Part 4**: [AI Overview](../4-ai/01-ai-overview.md).
