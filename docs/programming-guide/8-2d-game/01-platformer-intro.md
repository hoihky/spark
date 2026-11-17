# Platformer Introduction

This part applies **Parts 2–7** to a shippable side-scroller. The reference implementation is **`Platformer2DDemo`** (SparkDemo **#6**) — sources under `src/spark/demo/Platformer2DDemo.cpp` and `platformer2d/`. It is the **canonical facet sample**: entity spawn via `world.Entities().Create()`, assets via `world.Assets().RegisterTexture`, level scans via `world.Entities().ForEach` (see [ECS and Scene](../1-overview-architecture/06-ecs-and-scene.md)).

## What the demo proves

| System | Components / APIs |
|--------|-------------------|
| World | Tilemap or checker sprites, parallax, camera rig + shake |
| Player | `CharacterController2D`, semantic input, melee hitbox |
| Combat | `HealthComponent`, `AnimationHitbox2D`, enemies + `AiAgent` |
| Flow | `GameState`, `GameFlowTrigger`, goal triggers |
| HUD | `TextOverlay`, UI font mount |
| Audio | `SoundCue`, bundled + procedural clips |

**Component recipes:** [Part 7](../7-game-components-usage/01-2d-rendering.md) · **Rendering:** [Part 2](../2-2d-graphics/01-sprites.md).

## Demo class layout

```cpp
class Platformer2DDemo {
    PhysicsSubsystem physics;
    DemoRootCollection roots;
    // cached pointers: player, camera, game state, hud...
public:
    void Load(GameWorld& w, IEngineContext& context);
    void Simulate(const FrameTiming& timing, IEngineContext& context, GameWorld& world);
    void Render(Scene& scene, GameWorld& world, IEngineContext& context);
};
```

Wrap in `Game` or call helpers from `OnAttach` / `OnUpdate` / `OnRender`.

### C\#

```csharp
// Spark-sharp template: subclass Game, mirror Load/Simulate/Render from native sample.
public sealed class PlatformerGame : Game { }
```

### Python

```python
class PlatformerGame(Game):
    def on_attach(self, context): ...
    def on_update(self, timing, context):
        context.physics.simulate_2d(self.world, timing)
```

## Simulate order (critical)

SparkDemo runs **`Platformer2DDemo::Simulate`** then **`Game::OnUpdate` → `RunDefaultSlice`** (components, VFX, audio). Inside `Simulate`:

1. `PlayerInputComponent::Refresh` → set `CharacterController2D` intent (before physics)
2. `world.Physics(physics).Simulate2D(timing)`
3. `world.Simulation().RunGameplaySlice()` (deferred pickup destroys)
4. Bullets / enemy squad / gem presentation
5. `OnRender` — `world.Rendering().SubmitLitWithCamera`

Component ticks (FSM, animators, parallax, camera rig) run in **`RunDefaultSlice`**, not at the top of `Simulate`. Do not call `ProcessVfx` — the default slice already pumps VFX.

See [Engine Loop](../1-overview-architecture/04-engine-loop.md) for the full facet map.

## Minimal entry point

### C++

```cpp
#include "spark/engine/Engine.hpp"

int main() {
    Engine engine(Engine::NewGame<Platformer2DGame>());
    engine.Run();
    return 0;
}
```

### C\#

```csharp
public static class Program
{
    public static void Main() => SparkScriptHost.Run<PlatformerGame>();
}
```

Managed `PlatformerGame` mirrors `Load` / `Simulate` / `OnRender` from the native demo.

### Python

```python
from spark.host import run_game
run_game(PlatformerGame())
```

## SparkDemo keys

While in demo **#6**: same shell keys **TAB/H/F3**; in-level controls from `InputActionMap` (WASD, Space jump, J attack).

Next: [Project Setup](02-project-setup.md).
