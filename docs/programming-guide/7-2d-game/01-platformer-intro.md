# Platformer Introduction

We walk through the **2D platformer** built into SparkDemo (`Platformer2DDemo`, launcher item **#6**) — source in `src/spark/demo/Platformer2DDemo.cpp` and `src/spark/demo/platformer2d/`. It is a complete side-scroller with:

- Kenney tile atlas (checkerboard fallback when assets are missing)
- `ParallaxLayerComponent` multi-layer backgrounds with optional cloud drift
- `Camera2DRigComponent` smooth follow + `ScreenShakeComponent` on hurt and victory
- `CharacterController2DComponent` player (coyote time, jump buffer, one-way platforms)
- `InputActionMapComponent` + `PlayerInputComponent` semantic controls (WASD / arrows, Space, J)
- `AnimationHitbox2DComponent` melee during attack clip
- `TriggerVolume2DComponent` + `PickupComponent` gem collection
- `GameStateComponent` + `GameFlowTriggerComponent` goal → Victory flow
- Enemy squad with `AiAgentComponent` and bullets
- HUD via `TextOverlayComponent` and health bar sprites
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
    Spark::CharacterController2DComponent* playerController = nullptr;
    Spark::PlayerInputComponent* playerInput = nullptr;
    Spark::GameStateComponent* gameState = nullptr;
    Spark::ScreenShakeComponent* cameraShake = nullptr;
    Spark::TextOverlayComponent* hudText = nullptr;
    // ...

public:
    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);
    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context, Spark::GameWorld& world);
    void Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context);
};
```

Wrap it in a `Game` subclass or call the helpers from `OnAttach` / `OnUpdate` / `OnRender`.

## Frame Order (Simulate)

Understanding tick order prevents input and physics bugs:

1. `world.UpdateGameObjects` — `PlayerInputComponent` (50), FSM (100), animators (200), hitboxes (215), parallax (290), shake (295), camera rig (300)
2. Read `PlayerInputComponent` actions in demo `Simulate` (or rely on `Refresh` if custom)
3. Set `CharacterController2D` move/jump intent
4. `physics.Simulate2D` — rigid bodies, character controllers, trigger volumes
5. `PickupComponent::ProcessDeferredDestroys` — flush gem destroys after triggers
6. Enemy AI, bullets, VFX

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
constexpr float kPlayerMoveSpeed = 11.0F;
constexpr float kPlayerJumpSpeed = 13.2F;
constexpr float kGroundTopY = -1.0F;
constexpr float kGoalCenterX = 38.0F;
```

Study `Platformer2DDemo::Load` for texture registration, parallax layers, and input maps. Study `Simulate` for character control, shooting, enemy AI, and physics stepping order.

**Component reference:** [Game Component Reference](../1-overview-architecture/07-game-component-reference.md).

Next: [Project Setup](02-project-setup.md).
