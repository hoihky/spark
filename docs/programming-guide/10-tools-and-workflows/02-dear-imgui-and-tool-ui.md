# Dear ImGui and tool UI (demo #20)

SparkDemo **#20 / hotkey G** (`ImGuiShowcaseDemo`) enables **docking**, multi-viewport-style panels, and rapid parameter tweakers. Dear ImGui complements — does not replace — retained **`UiCanvasComponent`** for shipping HUDs.

**Architecture:** [`GUI_TOOLKIT_ARCHITECTURE.md`](../../GUI_TOOLKIT_ARCHITECTURE.md) · **Retained UI:** [UI and toolkits](../1-overview-architecture/08-ui-and-toolkits.md).

## Decision matrix

| Choose **Spark retained UI** | Choose **Dear ImGui** |
|------------------------------|----------------------|
| Pause menus, settings, diegetic UI | Profilers, live tuners, content browsers |
| Themed game fonts/colors | Docking tool chains, temporary debug |
| Serialize canvases in scenes | Internal tools, demo **#20** |
| `UiConsumesGamePointer()` contract | `#ifdef` editor builds only |

SparkShell disables some global overlays while ImGui showcase runs (no help HUD clash) — see `SparkShellDemo` mode checks.

## Concept: ImGui only in `OnRender`

#### C++

```cpp
void MyToolGame::OnRender(IRenderFrame&, IEngineContext& context) override {
    SubmitStandardLitSceneFromWorld(...);

    if (IImGuiLayer* imgui = context.TryGetImGuiLayer()) {
        if (imgui->IsAvailable() && imgui->IsEnabled()) {
            ImGui::Begin("Tuner");
            ImGui::SliderFloat("Exposure", &exposure, 0.1F, 3.0F);
            ImGui::End();
        }
    }
}
```

**Never** call `ImGui::Begin` inside `OnUpdate` — the engine calls `NewFrame` after update ([Engine loop](../1-overview-architecture/04-engine-loop.md)).

#### C\#

```csharp
if (context.TryGetImGuiLayer() is { } layer && layer.IsEnabled)
{
    ImGui.Begin("Tuner");
    // ...
    ImGui.End();
}
```

#### Python

```python
# ImGui rarely used from Python host today.
```

## Concept: prefer toolkit settings for hybrid games

#### C++

```cpp
#include "spark/ui/Ui.hpp"

Ui::UiToolkitSettings::SetPreferred(UiBackendKind::SparkNative);
// Tooling build may switch to DearImGui for specific canvases via UiSystem.
```

Gameplay pointer routing: if `UiConsumesGamePointer()` returns true, skip weapon raycasts that frame.

#### C\#

```csharp
UiToolkitSettings.SetPreferred(UiBackendKind.SparkNative);
```

#### Python

```python
pass
```

## Demo #20 workflow

1. Launch **G** from shell.  
2. Explore docking layouts, widget gallery, and theme toggles.  
3. Compare CPU cost vs a single `UiCanvas` pause menu in **#6**.

Use ImGui to prototype sliders for values you later bind to `MaterialComponent` or `ParticleEmitterComponent` fields in a retained panel.

## Build / ship

```bash
cmake -S . -B build -DSPARK_ENABLE_IMGUI=OFF
```

Strip tool code behind `#if SPARK_ENABLE_IMGUI` or separate CMake target so Release player builds stay lean.

Next: [Spark editor direction](03-spark-editor-direction.md).
