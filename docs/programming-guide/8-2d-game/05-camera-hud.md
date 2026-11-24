# Camera and HUD

Follow camera, parallax, flow state, HUD text, and optional GPU minimap — as wired in `Platformer2DDemo` and P0 (**F**). See [Part 7 — Camera](../7-game-components-usage/05-camera.md).

## ECS Camera Rig

Replace manual `Camera2D` position lerping with `Camera2DRigComponent` on a dedicated camera object. Add `ScreenShakeComponent` on the **same** object so shake offsets are applied inside the rig before submit.

### C++

```cpp
mainCameraGo = world.CreateGameObject();
auto* cam2d = mainCameraGo->AddComponent<Camera2DComponent>();
cam2d->SetHalfExtentY(6.5F);

auto* rig = mainCameraGo->AddComponent<Camera2DRigComponent>();
rig->SetMode(Camera2DRigMode::FollowTarget);
rig->SetTarget(playerObject);
rig->SetFollowSmoothRate(8.0F);
rig->SetLookAheadScale(0.12F);

cameraShake = mainCameraGo->AddComponent<ScreenShakeComponent>();
```

### C\#

```csharp
var camGo = world.CreateGameObject("MainCamera");
camGo.AddCamera2D(6.5f);
var rig = camGo.AddCamera2DRig();
rig.SetMode(Camera2DRigMode.FollowTarget).SetTarget(player).SetFollowSmoothRate(8f);
camGo.AddScreenShake();
```

### Python

```python
from spark._generated.component_kind import ComponentKind
cam = world.create_game_object("MainCamera")
cam.add_camera_2d(6.5)
cam.get_or_add_default_component(ComponentKind.Camera2DRig)
cam.get_or_add_default_component(ComponentKind.ScreenShake)
```

### Screen shake triggers

### C++

```cpp
// Player hurt:
cameraShake->AddImpulse({0.14F, -0.08F}, 0.2F, 32.0F);

// Goal reached (in GameStateComponent transition callback):
cameraShake->AddImpulse({0.22F, 0.14F}, 0.35F, 26.0F);
```

`Camera2DRigComponent` reads `ScreenShakeComponent::GetOffset()` automatically — do not add shake offset manually in render code.

### C\#

```csharp
cameraShake.AddImpulse(new Vector2(0.14f, -0.08f), 0.2f, 32f);
```

## Parallax Backgrounds

Attach `ParallaxLayerComponent` to each distant sprite layer. Wire the camera reference after the camera exists:

### C++

```cpp
bgMountains->AddComponent<SpriteComponent>(mountainsTex, ...);
auto* parallax = bgMountains->AddComponent<ParallaxLayerComponent>();
parallax->SetFactorX(0.18F);
parallax->SetCameraReference(mainCameraGo);
parallax->SetAnchorWorld({kBackgroundAnchorX, 0.0F, 0.0F});

// Optional cloud sway:
parallax->SetDriftMode(ParallaxDriftMode::SineHorizontal);
parallax->SetDriftAmplitude(0.35F);
parallax->SetDriftFrequencyHz(0.22F);
```

Lower `factorX` = slower scroll (farther away). Parallax runs at priority **290**, before shake and rig.

### C\#

```csharp
bg.AddParallaxLayer().SetFactorX(0.18f).SetCameraReference(mainCameraGo);
```

## Game Flow State

Centralize win/lose/pause in `GameStateComponent` on a manager object:

### C++

```cpp
gameFlowGo = world.CreateGameObject();
gameState = gameFlowGo->AddComponent<GameStateComponent>(GameFlowState::Playing);
gameState->SetOnTransition([](GameFlowState, GameFlowState next, GameObject&) {
    if (next == GameFlowState::Victory) {
        // celebration VFX, BGM sting, enable goal glow
    }
});

// Goal trigger on separate object:
goalTriggerGo->AddComponent<TriggerVolume2DComponent>(
    TriggerVolume2DShape::Box, Vector2{goalHalfW, goalHalfH});
auto* goalFlow = goalTriggerGo->AddComponent<GameFlowTriggerComponent>();
goalFlow->SetSource(GameFlowTriggerSource::TriggerEnter);
goalFlow->SetTargetState(GameFlowState::Victory);
goalFlow->SetInstigatorNameFilter("Player");
goalFlow->SetStateOwner(gameFlowGo);
```

### C\#

```csharp
var flow = world.CreateGameObject("GameFlow");
flow.AddGameState(GameFlowState.Playing);
goal.AddGameFlowTrigger().SetTargetState(GameFlowState.Victory).SetStateOwner(flow);
```

Query state in HUD and gameplay:

### C++

```cpp
const bool victory = gameState->IsState(GameFlowState::Victory);
```

## HUD Text

### C++

```cpp
hudText->SetText(Utf8String(std::format(
    "2D platformer {} | HP {:.0f} | gems {}/{}",
    gameState->IsState(GameFlowState::Victory) ? "— GOAL!" : "",
    playerHealth->GetCurrentHealth(),
    gemsCollected, kGemCount).c_str()));
```

### C\#

```csharp
hud.GetComponent<TextOverlayComponent>()?.SetText(
    $"gems {gemsCollected}/{kGemCount} | {gameState.CurrentState}");
```

## Render Submit

### C++

```cpp
const Matrix4 viewProj = camera.ViewProjection(static_cast<float>(fbW), static_cast<float>(fbH));
Vector3 pr{}, pu{};
camera.BillboardBasisWorld(pr, pu);

world.Rendering().SubmitLit(
    GetWorld(), context, viewProj, camera.position,
    Vector3{0.30F, 0.86F, 0.36F}.Normalized(),
    Vector3{1.0F, 0.98F, 0.95F}, 0.85F,
    Vector3{0.16F, 0.18F, 0.24F},
    false, pr, pu, sceneTimeSeconds,
    SceneSpriteSortMode::SortOrderThenWorldY);
```

When using `Camera2DRigComponent`, read the rig's resolved `Camera2D` pose for submit (the demo copies rig output into its `Camera2D` struct each frame).

## Minimap (GPU composite view)

For top-down games, prefer `Scene2DCompositeViewComponent` over hand-drawn UI quads:

1. `CreateMinimapRenderTexture()` — offscreen **RGBA8** target (compatible with UI atlas copy).
2. `CreateMinimapHudPlaceholderTexture()` — registers a layer in `SceneRenderParams::uiTextures`.
3. `context.TryGetRenderTargetService()->CreateRenderTarget(rt)` — GPU framebuffer when using Vulkan.
4. On a manager object: set feature `Minimap`, `target`, `hudTexture`, screen rect, and sync `SetWorldCapture(center, halfExtent)` each frame from player + grid bounds.
5. `PatchScene2DMinimapHud` draws the player dot aligned with capture ortho.

Disable GPU capture to fall back to CPU walkability tint (`RebuildMinimapTextureFromGameplayGrid`).

Details: [Runtime limits — composite views](08-scene2d-runtime-limits.md), [Gameplay API — minimap](09-2d-gameplay-api-guide.md#10-minimap-and-composite-views). Reference: `GameFlow2DProductPathDemo` (**F**).

### C\#

```csharp
SceneSubmit.world.Rendering().SubmitLit(World, ctx, viewProj, ...);
```

## `CameraBounds2DComponent`

Clamp the rig so the view never leaves the level:

### C++

```cpp
#include "spark/ecs/components/camera/CameraBounds2DComponent.hpp"

auto* bounds = cameraGo->AddComponent<CameraBounds2DComponent>();
bounds->SetWorldMin({minX, minY});
bounds->SetWorldMax({maxX, maxY});
```

## Floating damage numbers (world-space)

Spark does **not** ship a `WorldSpaceUiCanvas`. Choose between **screen-projected text** (recommended for readable numbers) and **world sprites** (stylistic digit atlases).

| Path | When to use | Pipeline |
|------|-------------|----------|
| **Screen-projected text** | Damage/heal numbers, crit text, pickup “+10” | `FloatingCombatTextPresenter` → `SceneRenderParams::screenOverlayTexts` via `TryWorldToScreenPixels` |
| **World sprites** | Pixel fonts baked into atlas, heavy motion blur, strict depth sort with actors | Pooled `SpriteComponent` quads at hit `Transform`; animate in world XY; counts toward sprite cap |
| **UiCanvas / `TextOverlay`** | HUD bars, menus, static labels | **Not** for per-hit world popups — both are **screen-pixel** retained or fixed coordinates |

### Screen-projected path (C++)

Call **after** `FillLit` (which clears UI draw lists) and **before** present:

```cpp
#include "spark/gameplay/FloatingCombatTextPresenter.hpp"
#include "spark/gameplay/GameplayDamageEvent.hpp"

FloatingCombatTextPresenter floatingText{};

// On damage signal (use victim world position + optional hit offset):
void OnHit(const Vector3& worldAt, float applied) {
    floatingText.SpawnDamage(worldAt, applied);
}

// Each frame:
floatingText.Advance(timing.deltaTimeSeconds);
world.Rendering().FillLit(world, ctx, viewProj, ...);
floatingText.AppendDraws(params, static_cast<float>(fbW), static_cast<float>(fbH));
```

`AppendDraws` uses the same `params.viewProjection` as the frame’s world draw. Numbers rise in **world +Y**, then project each frame so they track a moving camera.

Lower-level projection without the presenter:

```cpp
#include "spark/scene/camera/WorldSpaceScreenProject.hpp"

float sx = 0.0F;
float sy = 0.0F;
if (TryWorldToScreenPixels(params.viewProjection, hitWorld, fbW, fbH, sx, sy)) {
    ScreenTextDraw d{};
    d.x = sx;
    d.y = sy;
  // ...
    d.paintOrder = params.NextUiPaintOrder();
    params.screenOverlayTexts.PushBack(MoveTemp(d));
}
```

`Camera2D::TryWorldXYToScreen` is the 2D convenience wrapper around the same math.

### Sprite path (outline)

1. Pool inactive `GameObject`s with `SpriteComponent` + digit strip UVs (or one glyph per spawn).
2. On `SignalId::DamageApplied`, activate at `target->GetWorldMatrix().translation` (or hurtbox center).
3. Tween world position / tint / scale in `OnUpdate`; deactivate when TTL expires.
4. Set `sortOrder` above terrain, below foreground; respect `Scene2DSubmitCaps` sprite budget.

### What is not supported yet

- Retained **world-space** UI layout (billboard panels, nameplates with layout boxes) — gap tracked in `SCENE_AND_RENDERING_GAPS.md`.
- Automatic wiring from `GameplayDamageEvent` — spawn popups in your combat listener or a small game system.

## HUD components

| Component | Use |
|-----------|-----|
| `TextOverlayComponent` | Simple strings (gems, state name) |
| `UiCanvasComponent` | Retained UI layout (menus, inventory shells) |
| `Scene2DCompositeViewComponent` | Minimap texture in HUD layer |
| `FloatingCombatTextPresenter` | Ephemeral world-anchored combat numbers (screen-projected) |

```cpp
#include "spark/ecs/components/ui/UiCanvasComponent.hpp"

menuRoot->AddComponent<UiCanvasComponent>();
```

Audio: attach `AudioListenerComponent` to the same object as `Camera2DRigComponent` so spatial cues follow the view.

See [10 — Components handbook](10-2d-components-handbook.md#camera2d).

Next: [Polish and Ship](06-polish.md).
