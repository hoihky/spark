# Camera and HUD

## ECS Camera Rig

Replace manual `Camera2D` position lerping with `Camera2DRigComponent` on a dedicated camera object. Add `ScreenShakeComponent` on the **same** object so shake offsets are applied inside the rig before submit.

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

### Screen shake triggers

```cpp
// Player hurt:
cameraShake->AddImpulse({0.14F, -0.08F}, 0.2F, 32.0F);

// Goal reached (in GameStateComponent transition callback):
cameraShake->AddImpulse({0.22F, 0.14F}, 0.35F, 26.0F);
```

`Camera2DRigComponent` reads `ScreenShakeComponent::GetOffset()` automatically — do not add shake offset manually in render code.

## Parallax Backgrounds

Attach `ParallaxLayerComponent` to each distant sprite layer. Wire the camera reference after the camera exists:

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

## Game Flow State

Centralize win/lose/pause in `GameStateComponent` on a manager object:

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

Query state in HUD and gameplay:

```cpp
const bool victory = gameState->IsState(GameFlowState::Victory);
```

## HUD Text

```cpp
hudText->SetText(Utf8String(std::format(
    "2D platformer {} | HP {:.0f} | gems {}/{}",
    gameState->IsState(GameFlowState::Victory) ? "— GOAL!" : "",
    playerHealth->GetCurrentHealth(),
    gemsCollected, kGemCount).c_str()));
```

## Render Submit

```cpp
const Matrix4 viewProj = camera.ViewProjection(static_cast<float>(fbW), static_cast<float>(fbH));
Vector3 pr{}, pu{};
camera.BillboardBasisWorld(pr, pu);

SubmitStandardLitSceneFromWorld(
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

## `CameraBounds2DComponent`

Clamp the rig so the view never leaves the level:

```cpp
#include "spark/ecs/components/camera/CameraBounds2DComponent.hpp"

auto* bounds = cameraGo->AddComponent<CameraBounds2DComponent>();
bounds->SetWorldMin({minX, minY});
bounds->SetWorldMax({maxX, maxY});
```

## HUD components

| Component | Use |
|-----------|-----|
| `TextOverlayComponent` | Simple strings (gems, state name) |
| `UiCanvasComponent` | Retained UI layout (menus, inventory shells) |
| `Scene2DCompositeViewComponent` | Minimap texture in HUD layer |

```cpp
#include "spark/ecs/components/ui/UiCanvasComponent.hpp"

menuRoot->AddComponent<UiCanvasComponent>();
```

Audio: attach `AudioListenerComponent` to the same object as `Camera2DRigComponent` so spatial cues follow the view.

See [10 — Components handbook](10-2d-components-handbook.md#camera2d).

Next: [Polish and Ship](06-polish.md).
