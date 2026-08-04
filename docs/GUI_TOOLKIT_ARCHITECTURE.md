# GUI toolkit architecture

Spark’s retained UI lives in **`spark/ui/`** — a retained tree with Abstract Factory backends (Spark native + Dear ImGui). Legacy `spark/gui/` was removed in Phase 6.

## Layers

| Layer | Role |
|-------|------|
| **`IUiElement` / `UiCanvasComponent`** | Retained tree on ECS entities; measure, arrange, paint, hit-test. |
| **`IUiControlsFactory`** | Abstract Factory: `SparkUiControlsFactory` vs `DearImguiControlsFactory`. |
| **`IUiBackend`** | Strategy: input routing + paint for one stack. |
| **`UiSystem`** | Facade: active backend, `ProcessInput` / `Paint`, factory access via `GetContext`. |
| **`SparkUiRenderer`** | Adapts `IUiRenderer` → `UiPaintContext` → `SceneRenderParams` (Vulkan screen UI pass over 2D/3D). |
| **`ImguiUiRenderer`** | Dear ImGui adapter (Phase 3+ full control paint). |

## Phase 6 (current)

- **Deleted `spark/gui/`** — legacy `GuiSystem`, `GuiCanvasComponent`, `Widget` tree removed
- **Infrastructure moved to `spark/ui/`** — `UiPaintContext`, `UiSpriteSlice`, `UiThemeCatalog`, `EditorLayoutStore`, `UiContextMenu`
- **All demos migrated** — shell launcher, editor, ImGui showcase, particle demo, scene editor 3D
- **Input routing fix** — context menu works without an enabled canvas (Scene Editor RMB menu).
- **`ShouldProcessSparkUiInput()`** — wired in `DearImguiUiBackend`; Spark-native router runs when preferred backend is `SparkNative`.
- **Layout metrics** — `PrepareUiCanvasFrame()` shared by input and paint paths so hit-testing matches painted layout.
- **`ComponentKind::GuiCanvas` removed** — use `UiCanvasComponent` (`ComponentKind::UiCanvas`)

## Phase 5

- **`UiTheme::SceneEditorDark()`** — editor chrome palette for `UiCanvasComponent`
- **`UiConsumesGamePointer()` / `UiPointerOverUi()`** — replace `GuiConsumesGamePointer` in migrated apps
- **`IDockWorkspace` layout API** — collapse toggles, `GetCenterBounds()`, width get/set
- **`SparkDockWorkspace`** — collapsible left/right panes, passthrough center hit-test
- **`SparkEditor` migration** — `EditorDockShell` + Hierarchy/Inspector/Project panels on `UiCanvasComponent` + `SparkDockWorkspace`
- **`ImGuiShowcaseDemo` migration** — retained `ImguiDockWorkspace` via `UiSystem::Paint` (overlay demo/metrics windows remain raw ImGui)

## Phase 4

- **`IList` / `IMultiSelectList` / `ITreeView`** — factory interfaces + `ListDesc`, `MultiSelectListDesc`, `TreeViewDesc`
- **`SparkList`**, **`SparkMultiSelectList`**, **`SparkTreeView`** — ported from legacy `spark/gui/` with scrollbars, keyboard nav, multi-select modifiers, tree expand/collapse
- **`ImguiList`**, **`ImguiMultiSelectList`**, **`ImguiTreeView`** — Dear ImGui `ListBox` / `Selectable` / `TreeNodeEx` paint path
- **`UiIntCallback`** + **`UiFrameVoidCallback`** — list selection and frame-aware row clicks
- **`SparkButton`** — `SetAccentSelected`, `SetOpaqueSurface`, `SetOnClickWithFrame` for list rows
- **`PanelDesc::centerInParent`** — centered launcher panel layout
- **Shell launcher migration** — `SparkShellDemo` menu uses retained `UiCanvasComponent` + `SparkList` instead of immediate-mode `Gui::Ui()` selectables

## Phase 3

- **`ImguiUiRenderer` widget API** — `Button`, `Checkbox`, `SliderFloat`, `BeginPanel` / `EndPanel`, scroll regions, and dock workspace host
- **Dear ImGui retained controls** — `ImguiButton`, `ImguiPanel`, `ImguiLabel`, `ImguiSeparator`, `ImguiScrollPanel`, `ImguiSlider`, `ImguiCheckBox` emit ImGui widgets during `Paint`
- **`ImguiDockWorkspace`** — `ImGui::DockSpace` + `DockBuilder` split for left / center / right panes
- **`DearImguiUiBackend`** — owns `ImGui::NewFrame` / `Render` via `OnEnginePreRender` / `OnEnginePostRender`; paints canvases through `ImguiUiRenderer`
- **`UiToolkitSettings`** — `SetPreferred(SparkNative | DearImGui)` swaps `UiSystem` backend (mirrors legacy `GuiToolkitSettings`)
- **`PaintUiCanvases(world, IUiRenderer&)`** — backend-neutral paint entry for Dear ImGui path

Switch backend:

```cpp
Ui::UiToolkitSettings::SetPreferred(Ui::UiBackendKind::DearImGui);
```

## Phase 2

- **`UiInputRouter` polish** — framebuffer cursor coords, scroll-wheel routing, modal canvas gating, cross-canvas focus clear, `keyCanvas` selection, `consumesGamePointer` during drag capture
- **`UiScrollWheelConsumed()`** — gate 3D camera zoom when UI ate the wheel (mirrors `GuiScrollWheelConsumed`)
- **Focus lifecycle** — parent-chain focus lookup, `OnFocusGained` / `OnFocusLost`, `ProcessKeyInput` on focused controls
- **`SparkScrollPanel`** — vertical stack with clipped viewport, scrollbar thumb drag, mouse wheel
- **Keyboard** — sliders use arrow keys; checkboxes toggle with Space when focused

## Building a retained canvas

```cpp
#include "spark/ui/Ui.hpp"

Ui::IUiControlsFactory& factory = Ui::UiSystem::Get().GetActiveBackendPtr()->GetControlsFactory();

Ui::PanelDesc panelDesc{};
panelDesc.id = Utf8String("tune");
panelDesc.title = Utf8String("Tuning");
panelDesc.width = 560.0F;
panelDesc.anchorRight = true;
auto panel = factory.CreatePanel(panelDesc);

Ui::ScrollPanelDesc scrollDesc{};
scrollDesc.id = Utf8String("scroll");
scrollDesc.height = 220.0F;
auto scroll = factory.CreateScrollPanel(scrollDesc);
// add children to scroll, then panel...
```

Each frame:

```cpp
ProcessUiCanvasesInput(world, input, fbW, fbH);
if (!UiScrollWheelConsumed()) {
    // apply camera zoom from scroll wheel
}
PaintUiCanvases(world, params, fbW, fbH);
```

## Roadmap

| Phase | Scope |
|-------|--------|
| **0** | Skeleton |
| **1** | Full Spark controls + PhysicsBallThrow3D demo |
| **2** | Input router polish, focus, scroll |
| **3** | ImGui control paint + `ImguiDockWorkspace` |
| **4** | Lists + shell launcher migration |
| **5** | Editor docking migration |
| **6** | Delete `spark/gui/` (complete) |
