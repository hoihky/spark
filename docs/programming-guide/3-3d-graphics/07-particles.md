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
    void CollectInstances(Array<SceneParticleInstance>& out) const;
};
```

Requires `TransformComponent` on the same entity (emission origin = world translation).

## Fire Emitter (from `ParticleDemo`)

```cpp
auto* fxGo = world.CreateGameObject();
fxGo->AddComponent<TransformComponent>()->SetTranslation({0, 1, 0});

auto* emitter = fxGo->AddComponent<ParticleEmitterComponent>();
emitter->SetEmitterEnabled(true);
emitter->SetMaxParticles(512);
emitter->SetEmissionRate(80.0F);
emitter->SetLifetime(0.3F, 0.9F);
emitter->SetStartEndSize(0.15F, 0.02F);
emitter->SetStartEndColor(
    Vector4{1.0F, 0.55F, 0.12F, 1.0F},
    Vector4{0.85F, 0.05F, 0.0F, 0.0F});
emitter->SetGravity({0, 2.0F, 0});
emitter->SetEmissionDirection({0, 1, 0});
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
};
```

See `ParticleDemo` (SparkDemo **#3**) for fire, smoke, sparks, and magic burst presets in `ParticleDemoDetail.hpp`.

Part 3 complete → **Part 4**: [AI Overview](../4-ai/01-ai-overview.md).
