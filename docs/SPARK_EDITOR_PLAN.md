# Spark Editor — Gap Analysis & Implementation Plan

Godot-style game editor for Spark: **2D and 3D**, built on the engine’s retained UI (`spark/ui/`) plus optional Dear ImGui chrome, delivered as a **separate executable** linking **`SparkEngine`**.

**Related:** [`GUI_EDITOR_ROADMAP.md`](GUI_EDITOR_ROADMAP.md) (GUI milestones E0–E6), [`ARCHITECTURE_AND_DEVELOPER_GUIDE.md`](ARCHITECTURE_AND_DEVELOPER_GUIDE.md), [`SCENE_AND_RENDERING_GAPS.md`](SCENE_AND_RENDERING_GAPS.md), demo **#12** `SceneEditor3DDemo` (prototype).

---

## 1. Executive summary

| Layer | Status | Editor impact |
|-------|--------|---------------|
| Engine loop / ECS / `GameWorld` | **Ready** | Core simulation model |
| Vulkan forward renderer | **Ready** | Viewport scissor for center pane; offscreen RT later |
| UI toolkit | **Usable** | Docking (`SparkDockWorkspace` / `IDockWorkspace`), panels, ImGui file menu |
| Scene serialization v4 | **Partial** | **28+** component handlers; text format (reads v3) |
| `SceneEditor3DDemo` | **Prototype** | Pick, translate gizmo, save/load — reference for viewport service |
| **`SparkEditor` (M0–M2)** | **In progress** | Dock shell, hierarchy/inspector, project browser, scene save/load |
| Project / asset DB | **Partial** | `project.spark` JSON, asset catalog scan; no GUID DB yet |
| Inspector property grid | **Partial** | Transform, Mesh, Material, PointLight widgets |
| Undo / PIE | **Partial** | `EditorCommandStack`; play mode reloads editor scene |

**Strategy:** `spark/editor/` services inside the engine repo; ship **`SparkEditor`** via root CMake (`SPARK_BUILD_SPARK_EDITOR=ON`) or slim **`editor-debug`** preset (shared `SparkEngine`).

---

## 2. What exists (reusable)

### Engine & ECS
- `IGame` / `Game` / `Engine::Run` — editor is an `IGame` implementation (`EditorGame`).
- `GameWorld`, `GameObject`, **51** `ComponentKind` values (+ `Unknown`), hierarchy via `SetParent`.
- `Scene` + `SubmitStandardLitSceneFromWorld` — no Vulkan in editor code.
- `FlyCamera`, `MeshRaycast`, lighting profiles, 2D/3D cameras.

### UI
- Full control set via `SparkUiControlsFactory`: `TreeView`, panels, scroll regions, sliders, lists.
- **Docking:** `SparkDockWorkspace` / `IDockWorkspace` — used by `EditorDockShell`.
- **Dear ImGui** file menu bar (`PaintFileMenuBar`) when `SPARK_ENABLE_IMGUI`.
- `UiCanvasComponent` + `UiConsumesGamePointer()` for viewport input gating.
- `EditorLayoutStore` — persists `editor_layout.ini` (dock widths, theme).

### Scene authoring prototype (`SceneEditor3DDemo`)
- Translate gizmo, ray pick, context menu place/delete, `spark_scene_v4` save/load (reads v3).
- **Port target:** `EditorViewport` service — largely ported.

### Serialization
- `SceneSerializer` / `ComponentSnapshotRegistry` — **28+** handlers including `GltfSceneSource`, `SpawnPoint`, `MultiMaterial`, tilemap, gameplay, UI canvas, and more.
- **`gltf_scene` v2** — prefab root + per-node mesh/material overrides (no expanded child entities); see §4.1.
- Runtime: `SceneManager`, `PrefabInstantiator`, `GameWorldAssetLoader`.

### Build
- `SPARK_BUILD_ENGINE_SHARED` — proven in `game_template/`, `samples/*`.
- `SPARK_BUILD_SPARK_EDITOR` — compiles `spark/editor/` into `SparkEngine` + `SparkEditor` exe.

### Scripting (future PIE)
- `SparkScriptHost`, `SparkInterop`, C# `HelloCsGame`.

---

## 3. Missing engine capabilities (by priority)

### 3.1 Critical (blocks Godot-like editor)

| Gap | Why it matters | Milestone |
|-----|----------------|-----------|
| **Editor product module** | `spark/editor/` services | **M0** ✓ |
| **`EditorApplication` shell** | Edit mode, dock UI, fly camera | **M0** ✓ |
| **Project file (`project.spark`)** | Open folder, main scene, asset roots | **M1** ✓ |
| **File menu scene workflow** | Open/Save scene, native pickers | **M1** ✓ |
| **Asset browser** | Prefabs & scenes list, double-click open | **M1** ✓ |
| **Hierarchy panel** | Tree bound to world; CRUD via commands | **M2** (partial) |
| **Inspector / property grid** | Typed widgets for core components | **M2** (partial) |
| **`EditorSelection` service** | Primary selection wired to inspector | **M2** ✓ |
| **Dock workspace** | `EditorDockShell` + left/center/right | **M0** ✓ |
| **Embedded viewport service** | `worldViewportScissor` + gizmo toolbar | **M2** (partial) |
| **Undo/redo command stack** | Transform, create, delete, properties | **M2** (partial) |
| **Serialization coverage** | Remaining kinds (`UiCanvas` in scenes, …) | **M3** |
| **Play-in-editor (PIE)** | World snapshot / reload on exit | **M2** (partial) |
| **Prefab instance overrides** | glTF child mesh/material edits persist | **M2** ✓ |

### 3.2 Important (v1 quality)

| Gap | Milestone |
|-----|-----------|
| Rotate/scale gizmos | M3 |
| Selection outline pass (not emissive pulse) | M3 |
| GPU pick buffer | M4 |
| Add/remove component UI | M3 |
| Multi-material inspector | M4 |
| Animation timeline | M4 (needs ANIMATION_3D) |
| Script attach + build console | M5 |
| Physics collider debug draw | M4 |
| Offscreen “Game” camera view | M5 |
| Asset GUID database | M2–M3 |

### 3.3 Nice-to-have (Godot parity)

Material graph, visual scripting, plugins, tilemap/terrain brushes, LSP, CI scene round-trip — see [`GUI_EDITOR_ROADMAP.md`](GUI_EDITOR_ROADMAP.md) E6+.

---

## 4. Scene save/load (editor)

### 4.1 glTF prefab instances

Placed glTF prefabs (`.sparkscene` with `gltf_scene`) **expand at runtime** into a visual subtree. The editor must not serialize that subtree as separate entities (causes duplicate meshes on reload).

| Concern | Approach |
|---------|----------|
| **Anti-duplication** | `ShouldCaptureSceneEntity` saves only **placed prefab roots**, not `GltfInstanceNode` descendants |
| **Persist edits** | `gltf_scene` **v2** payload embeds override records keyed by glTF node index |
| **Mesh overrides** | `mesh <nodeIndex> <albedoR> <albedoG> <albedoB>` |
| **Material overrides** | `mat <nodeIndex> [v4 "<libraryKey>"] <MaterialSlotSnapshot v1…>` |
| **Runtime tagging** | `GltfInstanceNodeComponent` stamped during `GltfSceneImporter::ImportNode` |
| **Implementation** | `GltfInstanceOverrides.cpp`, `GltfSceneSourceSnapshotHandler` |

Simple mesh prefabs (e.g. `crate.sparkscene` — single `mesh` on root) continue to round-trip via normal entity capture.

### 4.2 File menu & dialogs

- **File → Open Scene / Save Scene As** — native macOS pickers (`NativeFilePickerMac.mm`, `UTType` for `.sparkscene`).
- Dialogs run at end of **`OnUpdate`** (outside ImGui frame) via `ProcessPendingFileDialogs`.
- **Ctrl+S** saves to active path or project `main_scene` default under `assets/scenes/`.

### 4.3 Project workflow

- `project.spark` JSON: name, `assets_directory`, `main_scene`, layout path.
- **New / Open / Save Project** — folder picker + `EditorProject::TrySaveProjectFile`.
- Asset catalog rescans on project open; browser double-click opens `.sparkscene` files.

---

## 5. Architecture

```
┌─────────────────────────────────────────────────────────┐
│  SparkEditor (IGame: EditorGame)                         │
├─────────────────────────────────────────────────────────┤
│  spark/editor/panels   Hierarchy, Inspector, Project     │
├─────────────────────────────────────────────────────────┤
│  spark/editor services EditorApplication, Selection,     │
│                          Project, CommandStack, Viewport │
├─────────────────────────────────────────────────────────┤
│  spark/ui + Dear ImGui   retained panels + file menu     │
├─────────────────────────────────────────────────────────┤
│  SparkEngine             ECS, scene, render, audio, …      │
└─────────────────────────────────────────────────────────┘
```

### OOP principles
- **`IEditorPanel`** — single responsibility per dock panel.
- **`EditorContext`** — façade passed to panels (world, selection, project).
- **`EditorSelection`** — owns selection set; panels subscribe via callbacks.
- **`EditorProject`** — project path, settings, dirty flags.
- **Extensibility** — new panels implement `IEditorPanel`; new components add `IComponentSnapshotHandler` + inspector widget.

---

## 6. Roadmap & task tracking

| ID | Milestone | Goal | Exit criteria |
|----|-----------|------|---------------|
| **M0** | Foundation | Slim DLL + editor exe | `SparkEditor` runs, empty 3D scene + dock shell |
| **M1** | Project shell | Menu, project browser, layout | Open folder, persist panel widths, scene file menu |
| **M2** | Scene tools | Hierarchy, save, gizmos, PIE | Author `.sparkscene` with prefab overrides |
| **M3** | Authoring depth | More handlers, component UI | Full inspector coverage for shipped components |
| **M4** | Materials & animation | Texture slots, timeline | Material library UX |
| **M5** | PIE + C# scripts | Play stops, restores edit world | Script build console |
| **M6** | Product | New project wizard, user guide | CI round-trip |

### M0 — Foundation (**complete**)

- [x] `SPARK_BUILD_SPARK_EDITOR` CMake option
- [x] `spark/editor/` module: `EditorApplication`, `EditorSelection`, `EditorProject`
- [x] `SparkEditor` executable
- [x] Dock shell: hierarchy + inspector + project panels
- [x] Default 3D scene (ground + sun + sample cube), fly camera viewport

### M1 — Project & scene files (**complete**)

- [x] `project.spark` JSON read/write (`EditorProject`)
- [x] `ProjectBrowserPanel` + asset catalog (`SceneEditorAssetCatalog`)
- [x] `EditorLayoutStore` integration for dock widths + theme
- [x] File menu: New/Open/Save project; Open/Save scene; Save Scene As
- [x] Native folder/file pickers (macOS `UTType` for `.sparkscene`)
- [x] Double-click `.sparkscene` in asset browser to open

### M2 — Scene tools (**in progress**)

- [x] `HierarchyPanel` ↔ `GameWorld` tree
- [x] `EditorViewport` pick + translate gizmo (ported from demo)
- [x] `EditorCommandStack` — transform, create, delete, duplicate, material/mesh/light edits
- [x] Inspector widgets: Transform, Mesh, Material, PointLight
- [x] Scene save/load with glTF prefab override serialization (v2)
- [x] Play mode with scene reload on exit
- [ ] Hierarchy multi-select
- [ ] Rotate/scale gizmos
- [ ] Add/remove component menu

### M3 — Authoring depth

- [ ] Remaining `ComponentSnapshotHandler` coverage for editor-authored scenes
- [ ] Selection outline render pass
- [ ] Prefab variant / nested prefab workflow

---

## 7. Project layout

```text
spark/
├── include/spark/editor/
├── src/spark/editor/
├── spark_editor/               # SparkEditor executable
├── assets/project.spark        # default project template
└── docs/SPARK_EDITOR_PLAN.md
```

### Build editor

```bash
cmake --preset editor-debug
cmake --build cmake-build-editor --target SparkEditor
./cmake-build-editor/spark_editor/SparkEditor
```

Or from a unified build:

```bash
cmake --build build -j --target SparkEditor
```

---

## 8. Godot comparison (target v1)

| Godot | Spark Editor target |
|-------|---------------------|
| Scene tree dock | Hierarchy panel |
| Inspector | Inspector panel (partial widgets) |
| 3D viewport + gizmo | `EditorViewport` translate gizmo |
| FileSystem | Project browser + asset catalog |
| `.tscn` | `.sparkscene` (`spark_scene_v4` text) |
| Play (F5) | Play mode (P key) with reload |
| Script editor | TextArea + `dotnet build` (M5) |
| Undo | `EditorCommandStack` (partial) |

---

## 9. Immediate next steps

1. **M2** — Multi-select, rotate/scale gizmos, add-component menu.
2. **M3** — Extend serialization handlers; selection outline pass.
3. **M2** — Asset GUID database (replace path-only catalog).
4. Add **component handlers** incrementally — each unlocks inspector + save.

Update this document at each milestone; cross-link task IDs with [`GUI_EDITOR_ROADMAP.md`](GUI_EDITOR_ROADMAP.md).
