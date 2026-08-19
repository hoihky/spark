# Platformer Introduction

We walk through the **2D platformer** built into SparkDemo (`Platformer2DDemo`, launcher item **#6**) — source in `src/spark/demo/Platformer2DDemo.cpp` and `src/spark/demo/platformer2d/`. It is a complete side-scroller with:

- Kenney tile atlas (checkerboard fallback when assets are missing)
- `Camera2DRigComponent` smooth follow
- `Rigidbody2DComponent` player + static platforms
- Enemy squad with `AiAgentComponent` and bullets
- HUD via `TextOverlayComponent`
- Goal reach detection and gem collection
- Sound cues and BGM via `TryLoadSoundClipFromBundledAsset`

## Class Design: `Platformer2DDemo`

The demo uses **Load / Simulate / Render** helpers (same pattern you can copy into your own `IGame`):

```cpp
class Platformer2DDemo {
    Spark::PhysicsSubsystem physics;
    Spark::DemoRootCollection roots;
    Spark::GameObject* playerObject = nullptr;
    Spark::TransformComponent* playerTr = nullptr;
    Spark::Rigidbody2DComponent* playerRb = nullptr;
    Spark::TextOverlayComponent* hudText = nullptr;
    // ...

public:
    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);
    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context, Spark::GameWorld& world);
    void Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context);
};
```

Wrap it in a `Game` subclass or call the helpers from `OnAttach` / `OnUpdate` / `OnRender`.

## Minimal External Game Entry Point

```cpp
#include "spark/engine/Engine.hpp"
#include "Platformer2DGame.hpp"

int main() {
    Spark::Engine engine(Spark::Engine::NewGame<Spark::Platformer2DGame>());
    engine.Run();
    return 0;
}
```

## Constants (from `platformer2d/Config.hpp`)

```cpp
constexpr float kRunSpeed = 9.0F;
constexpr float kJumpSpeed = 11.5F;
constexpr float kGroundTopY = -1.0F;
constexpr float kGoalMinX = 15.5F;
```

Study `Platformer2DDemo::Load` for texture registration, player spawn, and level geometry. Study `Simulate` for input, shooting, enemy AI, and physics stepping order.

Next: [Project Setup](02-project-setup.md).
