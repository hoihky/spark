# Polish and Ship

## Juice: Screen Shake + Parallax

Screen shake and parallax are component-driven — no manual camera math in `Simulate`:

```cpp
// Hurt feedback:
cameraShake->AddImpulse({0.12F, -0.06F}, 0.18F, 30.0F);

// Victory (from GameStateComponent::SetOnTransition):
cameraShake->AddImpulse({0.22F, 0.14F}, 0.35F, 26.0F);
```

Tune parallax `factorX` per layer (sky 0.04, mountains 0.18, hills 0.32). Add `ParallaxDriftMode::SineHorizontal` on cloud layers for ambient motion without scripting.

## Sprite Animation Events

Footsteps and land dust can use marker-driven VFX instead of hard-coded frame checks:

```cpp
player->AddComponent<SpriteAnimationEventReceiverComponent>();
auto* recv = player->GetComponent<SpriteAnimationEventReceiverComponent>();
recv->AddMarker(1, 0.5F, "Footstep");  // run clip, midpoint

auto* vfx = player->AddComponent<SpriteAnimationEventVfxComponent>();
vfx->AddBinding("Footstep", "dust_puff");
```

See [2D Animation](../2-2d-graphics/04-2d-animation.md#sprite-animation-events).

## Sound

```cpp
auto* cue = playerObject->AddComponent<SoundCueComponent>();
if (justJumped) cue->Queue(SoundClip::CreateToneBlip(520, 0.07F, 0.5F));
```

Land dust SFX pairs well with `CharacterController2DComponent::WasGroundedLastFrame()` transition detection.

## Pause Menu (UI)

```cpp
#include "spark/ui/Ui.hpp"

auto* uiGo = world.CreateGameObject();
auto* canvas = uiGo->AddComponent<UiCanvasComponent>();

Ui::IUiControlsFactory& factory =
    Ui::UiSystem::Get().GetActiveBackendPtr()->GetControlsFactory();

Ui::PanelDesc panelDesc{};
panelDesc.id = Utf8String("pause");
panelDesc.title = Utf8String("Paused");
panelDesc.centerInParent = true;
panelDesc.width = 280.0F;
auto panel = factory.CreatePanel(panelDesc);

Ui::ButtonDesc btnDesc{};
btnDesc.id = Utf8String("resume");
btnDesc.label = Utf8String("Resume");
auto btn = factory.CreateButton(btnDesc);
Ui::UiVoidCallback resumeCb{};
resumeCb.fn = [](void*) { paused = false; };
btn->SetOnClick(resumeCb);
Ui::AdoptUiChild(*panel, MoveTemp(btn));

canvas->SetRoot(MoveTemp(panel));
```

Frame flow:

```cpp
ProcessUiCanvasesInput(GetScene(), input, fbW, fbH);
// ... fill params ...
PaintUiCanvases(GetWorld(), params, fbW, fbH);
```

## Replace Procedural Art

Swap `CreateCheckerboard` for `Texture2D::TryLoadFromFile("assets/tiles.png", ...)`.

## Ship Checklist

- [ ] Release build (`CMAKE_BUILD_TYPE=Release`)
- [ ] Bundle `assets/` beside executable
- [ ] Test on target DPI / resolution
- [ ] Verify `SPARK_BUILD_ASSETS_DIR` paths

Part 7 complete → **Part 8**: [FPS Introduction](../8-3d-game/01-fps-intro.md).
