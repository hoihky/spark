# Platformer Introduction

This part applies **Parts 2–7** to a shippable side-scroller. The reference implementation is **`Platformer2DDemo`** (SparkDemo **#6**) — sources under `src/spark/demo/Platformer2DDemo.cpp` and `platformer2d/`.

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

1. `world.UpdateGameObjects` — input (50), FSM (100), animators (200), parallax (290), camera (300)
2. Read `PlayerInputComponent` → set `CharacterController2D` intent
3. `physics.Simulate2D`
4. `PickupComponent::ProcessDeferredDestroys`
5. Bullets / enemy AI
6. `SimulateGameAi` if using nav agents

Skipping step 3 before gameplay rules causes one-frame overlap bugs.

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
