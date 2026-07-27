# GUI toolkit architecture

Spark exposes **one portable control API** (`spark/gui/api/GuiApi.hpp`) and swaps **backends** with the Strategy pattern.

## Layers

| Layer | Role |
|-------|------|
| **`IGuiFrame` / `Gui::Ui()`** | Stable API for game/editor code (buttons, sliders, panels). |
| **`IGuiBackend`** | Strategy: input routing, paint, engine hooks, `IGuiFrame` implementation. |
| **`GuiSystem`** | Facade: active backend, `ProcessInput` / `Paint`, engine pre/post render. |
| **Spark native retained** | `GuiCanvasComponent` + `Widget` tree (`ProcessSparkRetainedCanvases*`); still supported for rich editor chrome. |
| **Dear ImGui** | `DearImGuiGuiBackend` + `IImGuiLayer` Vulkan draw path. |

## Switching backends

```cpp
#include "spark/gui/toolkit/GuiToolkitSettings.hpp"

Gui::GuiToolkitSettings::SetPreferred(Gui::GuiToolkitKind::SparkNative);  // or DearImGui
```

`SetPreferred` updates `GuiSystem` so input and engine ImGui frames follow the active strategy.

## Portable UI (preferred for new code)

```cpp
#include "spark/gui/api/GuiApi.hpp"

Gui::GuiFrameContext ctx{};
ctx.renderParams = &params;
ctx.input = &context.GetInput();
ctx.uiFont = world.GetUiFont().Get();
ctx.framebufferWidth = fbW;
ctx.framebufferHeight = fbH;
ctx.deltaTimeSeconds = timing.deltaTimeSeconds;

Gui::GuiSystem::Get().BeginImmediateFrame(ctx);
Gui::IGuiFrame& ui = Gui::Ui();
if (ui.Button("apply", "Apply")) { /* ... */ }
ui.SliderFloat("exposure", "Exposure", exposure, 0.1F, 3.0F);
Gui::GuiSystem::Get().EndImmediateFrame();
```

Call `PaintGuiCanvases` when using **retained** canvases on the Spark native backend (e.g. scene editor context menu overlay).

## Demos

SparkDemo modes build interactive UI with **`Gui::Ui()`** + `DemoGui::MakeFrameContext` (`include/spark/demo/DemoGuiFrame.hpp`):

- **Launcher** (`SparkShellDemo`) — demo list + theme cycle
- **Physics / particles** — right-side tuning panels
- **ImGui showcase** — tool panels and menus via portable API; dock root still uses Dear ImGui `DockSpace` (no portable equivalent yet)

**Scene editor** still calls `PaintGuiCanvases` so the global **context menu** (retained engine overlay) can paint. It does not build tuning panels from `Widget` trees anymore in other demos.

`MountUiFont` lives in `ShellDemoSceneUtil.hpp` for HUD / portable text.

## SOLID mapping

- **S**: `IGuiBackend` = one stack; `IGuiFrame` = control vocabulary.
- **O**: New backends implement `IGuiBackend` without changing `Gui::Ui()` callers.
- **L**: `SparkNativeGuiBackend` and `DearImGuiGuiBackend` are interchangeable via `GuiSystem`.
- **I**: Engine depends on `GuiSystem`, not ImGui or `Widget`.
- **D**: Demos depend on `IGuiFrame`, not concrete ImGui or `Button` widgets.

## Files

- `include/spark/gui/api/` — public facade and interfaces
- `src/spark/gui/api/` — backend implementations
- `include/spark/gui/internal/SparkNativeRetainedGuiBridge.hpp` — retained canvas entry points
