# Polish and Ship

## Sound

```cpp
auto* cue = playerObject->AddComponent<SoundCueComponent>();
if (justJumped) cue->Queue(SoundClip::CreateToneBlip(520, 0.07F, 0.5F));
```

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
