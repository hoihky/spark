# Particles

## Class Design: `ParticleEmitterComponent`

CPU-simulated billboard particles collected into `SceneRenderParams::particles`:

```cpp
class ParticleEmitterComponent final : public GameComponent {
public:
    void SetEmissionRate(float rate) noexcept;
    void SetLifetime(float minSec, float maxSec) noexcept;
    void SetStartEndSize(float start, float end) noexcept;
    void SetGravity(const Vector3& g) noexcept;
    void SetEmissionDirection(const Vector3& dir) noexcept;
    void CollectInstances(Array<SceneParticleInstance>& out) const;
};
```

Requires `TransformComponent` on the same entity (emission origin = world translation).

## Fire Emitter Example

```cpp
auto* fxGo = world.CreateGameObject();
fxGo->AddComponent<TransformComponent>()->SetTranslation({0, 1, 0});

auto* emitter = fxGo->AddComponent<ParticleEmitterComponent>();
emitter->SetEmissionRate(80.0F);
emitter->SetLifetime(0.3F, 0.9F);
emitter->SetStartEndSize(0.15F, 0.02F);
emitter->SetGravity({0, 2.0F, 0});
emitter->SetEmissionDirection({0, 1, 0});
```

Enable particle collection in submit:

```cpp
SubmitStandardLitSceneFromWorld(..., enableParticles = true, camRight, camUp, sceneTime);
```

## `SceneParticleInstance`

```cpp
struct SceneParticleInstance {
    Vector3 position{};
    float size = 0.1F;
    Vector4 color{1, 1, 1, 1};
};
```

See `ParticleDemo` for colored bursts and muzzle flash patterns.

Part 3 complete → **Part 4**: [AI Overview](../4-ai/01-ai-overview.md).
