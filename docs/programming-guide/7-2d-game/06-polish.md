# Polish and Ship

## Sound

```cpp
auto* cue = playerObject->AddComponent<SoundCueComponent>();
if (justJumped) cue->Queue(SoundClip::CreateToneBlip(520, 0.07F, 0.5F));
```

## Pause Menu (GUI)

```cpp
#include "spark/gui/GuiControls.hpp"
#include "spark/ecs/components/GuiCanvasComponent.hpp"
#include "spark/gui/GuiScene.hpp"

auto* uiGo = world.CreateGameObject();
auto* canvas = uiGo->AddComponent<GuiCanvasComponent>();
auto root = MakeUnique<Gui::StackPanel>();
auto btn = MakeUnique<Gui::Button>();
btn->SetLabel(Utf8String("Resume"));
btn->SetOnClick([] { paused = false; });
root->AddChild(MoveTemp(btn));
canvas->SetRoot(MoveTemp(root));
```

Frame flow:

```cpp
ProcessGuiCanvasesInput(GetScene(), input, fbW, fbH);
// ... fill params ...
PaintGuiCanvases(GetWorld(), params, fbW, fbH);
```

## Replace Procedural Art

Swap `CreateCheckerboard` for `Texture2D::TryLoadFromFile("assets/tiles.png", ...)`.

## Ship Checklist

- [ ] Release build (`CMAKE_BUILD_TYPE=Release`)
- [ ] Bundle `assets/` beside executable
- [ ] Test on target DPI / resolution
- [ ] Verify `SPARK_BUILD_ASSETS_DIR` paths

Part 7 complete → **Part 8**: [FPS Introduction](../8-3d-game/01-fps-intro.md).
