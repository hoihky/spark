# VFX and `.sparkvfx`

Spark ships **GPU particle emitters** (`ParticleEmitterComponent`) and **authored effect players** (`VfxPlayerComponent`) that load `.sparkvfx` definitions. Demos: **#3** `VfxShowcaseDemo`, **#25** `VfxShowcase2DDemo` (sprite layer).

**Part 7:** [AI, audio & world](../7-game-components-usage/10-ai-audio-world.md) (VfxPlayer) · **Particles:** [ch.07](07-particles.md).

## When to use which

| Approach | Best for |
|----------|----------|
| `ParticleEmitterComponent` | Procedural bursts, sprite-layer juice, continuous emitters |
| `VfxPlayerComponent` + `.sparkvfx` | Tuned RPG/FPS presets, save/load tuning in showcase |
| `SpriteLighting2D` modes | Hit flash, dissolve, outline on quads ([2D lighting](../2-2d-graphics/05-2d-lighting.md)) |

## Concept: world-space particles

### C++

```cpp
#include "spark/ecs/components/ParticleEmitterComponent.hpp"

GameObject* burst = world.CreateGameObject();
burst->AddComponent<TransformComponent>()->SetTranslation(hitPoint);
auto* emitter = burst->AddComponent<ParticleEmitterComponent>();
emitter->SetRenderSpace(ParticleRenderSpace::World);
emitter->SetEmissionRate(120.0F);
emitter->SetLifetimeSeconds(0.4F);
```

### C\#

```csharp
impact.AddParticleEmitter()
    .SetRenderSpace(ParticleRenderSpace.World)
    .SetEmissionRate(120f);
```

### Python

```python
from spark._generated.component_kind import ComponentKind
impact.get_or_add_default_component(ComponentKind.ParticleEmitter)
```

## Concept: sprite-layer particles (2D)

Same component, different render space — drawn with the 2D lit pass (platformer pickups, UI-adjacent FX).

### C++

```cpp
emitter->SetRenderSpace(ParticleRenderSpace::SpriteLayer);
```

Bundled presets under `assets/vfx/*_2d.sparkvfx` — see demo **#25**.

### C\#

```csharp
pickup.AddParticleEmitter().SetRenderSpace(ParticleRenderSpace.SpriteLayer);
```

## Concept: `VfxPlayerComponent`

### C++

```cpp
#include "spark/ecs/components/vfx/VfxPlayerComponent.hpp"

go->AddComponent<VfxPlayerComponent>()->Play("assets/vfx/explosion_01.sparkvfx");
```

Showcase demo **#3** lets you tune parameters, then save updated `.sparkvfx` files for your game.

### C\#

```csharp
go.AddVfxPlayer().Play("assets/vfx/explosion_01.sparkvfx");
```

### Python

```python
go.get_or_add_default_component(ComponentKind.VfxPlayer)
```

## Concept: pairing with gameplay events

Wire one-shots to animation markers or combat:

### C++

```cpp
// Sprite marker → SoundCue / VfxPlayer (platformer)
recv->AddMarker(2, 0.1F, "MuzzleFlash");
vfx->AddBinding("MuzzleFlash", "assets/vfx/muzzle_2d.sparkvfx");
```

See [2D animation events](../2-2d-graphics/04-2d-animation.md) and [Sound cues](../6-sound/03-cues.md).

### C\#

```csharp
receiver.AddMarker(2, 0.1f, "MuzzleFlash");
```

## Budgets

Particle counts feed `SceneRenderParams::particles` (8192 default cap). See [2D runtime limits](../8-2d-game/08-scene2d-runtime-limits.md) for composite/minimap unrelated caps.

## Demo workflow (authoring)

1. Run SparkDemo **#3** or **#25**.  
2. Pick a built-in effect; tune spawn rate, color, lifetime.  
3. Save `.sparkvfx` into your game `assets/vfx/`.  
4. Reference path from `VfxPlayerComponent::Play` or spawn emitter presets in code.

Next: [Environment stack](09-environment-stack.md) · [Post-process, fog, volumes](10-post-process-fog-volumes.md) (then Part 4).
