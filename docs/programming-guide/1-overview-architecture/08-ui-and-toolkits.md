---
title: UI and Toolkits
order: 8
---

# UI and Toolkits

Spark supports **two complementary UI stacks** for tools, menus, and HUD:

| Stack | Style | Best for |
|-------|-------|----------|
| **Spark retained GUI** (`spark/gui/`) | Widget tree, CPU paint → `SceneRenderParams` | In-game menus, editor chrome, themed controls |
| **Dear ImGui** (`spark/imgui/`, optional) | Immediate-mode, docking branch | Rapid tool panels, debug windows, internal editors |

Both can coexist in one build when `SPARK_ENABLE_IMGUI=ON` (default). Use **`GuiToolkitSettings`** to choose which stack receives pointer/keyboard routing for a mode.

## Spark Retained GUI (default)

Attach a **`GuiCanvasComponent`** to a `GameObject`, set a root **`Widget`** tree, then each frame:

```cpp
#include "spark/gui/GuiScene.hpp"

Spark::ProcessGuiCanvasesInput(world, input, fbW, fbH, contentScaleX, contentScaleY);
// … after building SceneRenderParams …
Spark::PaintGuiCanvases(world, params, fbW, fbH);
```

- **Input:** hit testing, focus, scroll, popups — `ProcessGuiCanvasesInput`
- **Paint:** emits `screenRects` / `screenTexts` into `SceneRenderParams` — `PaintGuiCanvases`
- **Themes:** `GuiThemeCatalog` presets (`ClassicMint`, `SceneEditorDark`, …)
- **Editor docking:** `DockManager`, `DockFrameLayout` under `spark/gui/docking/`

See also [`ARCHITECTURE_AND_DEVELOPER_GUIDE.md`](../../../ARCHITECTURE_AND_DEVELOPER_GUIDE.md) §12 and [`GUI_EDITOR_ROADMAP.md`](../../../GUI_EDITOR_ROADMAP.md).

## Dear ImGui (optional)

CMake option **`SPARK_ENABLE_IMGUI`** (default **ON**) links the **docking** branch of Dear ImGui with GLFW + Vulkan backends (`cmake/SparkImGui.cmake`).

### Access from gameplay

```cpp
#include "spark/imgui/IImGuiLayer.hpp"
#include "spark/gui/toolkit/GuiToolkitSettings.hpp"

void MyToolMode::Enter(Spark::IEngineContext& context) {
    Spark::Gui::GuiToolkitSettings::SetPreferred(Spark::Gui::GuiToolkitKind::DearImGui);
    if (Spark::IImGuiLayer* imgui = context.TryGetImGuiLayer()) {
        imgui->SetEnabled(imgui->IsAvailable());
    }
}

void MyToolMode::Leave(Spark::IEngineContext& context) noexcept {
    if (Spark::IImGuiLayer* imgui = context.TryGetImGuiLayer()) {
        imgui->SetEnabled(false);
    }
    Spark::Gui::GuiToolkitSettings::SetPreferred(Spark::Gui::GuiToolkitKind::SparkNative);
}
```

Build ImGui UI in **`OnRender`** (after the engine calls `IImGuiLayer::BeginFrame` / `ImGui::NewFrame`):

```cpp
#if SPARK_ENABLE_IMGUI
#include <imgui.h>

void MyToolMode::OnRender(Spark::IRenderFrame&, Spark::IEngineContext& context) {
    if (ImGui::Begin("Inspector")) {
        ImGui::SliderFloat("Exposure", &exposure, 0.1F, 3.0F);
    }
    ImGui::End();
    // … set SceneRenderParams for 3D backdrop …
}
#endif
```

### Engine frame order (ImGui enabled)

```mermaid
sequenceDiagram
    participant E as Engine
    participant G as IGame
    participant I as IImGuiLayer
    participant V as VulkanRenderer
    E->>E: PollEvents + input BeginFrame
    E->>G: OnUpdate (simulation; may enable ImGui)
    E->>I: BeginFrame (NewFrame)
    E->>G: OnRender (build ImGui windows)
    E->>I: EndFrame (Render)
    E->>V: PresentFrame (record ImGui after screen UI)
```

**Rules:**

1. Do **not** call `ImGui::Begin` from `OnUpdate` — `NewFrame` runs after update.
2. Call **`InstallPlatformCallbacks`** once after `GlfwInput::WireToWindow` (the engine does this automatically) so ImGui chains GLFW mouse/key/scroll handlers.
3. Use **`WantsCaptureMouse()` / `WantsCaptureKeyboard()`** (previous frame) to gate game pointer when both stacks are compiled in.

### Demo

**SparkDemo** launcher item **19 — Dear ImGui tools (docking)** (hotkey **G**): docked Hierarchy / Inspector / Console, optional `ShowDemoWindow`, 3D backdrop. Source: `ImGuiShowcaseDemo` (`include/spark/demo/ImGuiShowcaseDemo.hpp`).

### Disable ImGui

```bash
cmake -S . -B build -DSPARK_ENABLE_IMGUI=OFF
```

`IImGuiLayer` becomes a null object; `SPARK_ENABLE_IMGUI=0` in `spark/config.hpp`.

## Choosing a toolkit

| Use Spark GUI | Use Dear ImGui |
|---------------|----------------|
| Shipped game menus with consistent theming | Internal tools, profilers, temp editors |
| `GuiCanvasComponent` in saved scenes | Docking-heavy IDE-style layouts |
| Full widget catalog (`TreeView`, `Dialog`, …) | ImGui ecosystem widgets / `imgui_demo.cpp` |

**`GuiToolkitSettings::ShouldProcessSparkGuiInput()`** returns `false` when the preferred toolkit is `DearImGui`, so retained canvases skip hit testing while ImGui is active.

Next: [Sprites](../2-2d-graphics/01-sprites.html) (Part 2) or continue with [Game Component Reference](07-game-component-reference.html) for `GuiCanvasComponent`.
