# Spark Editor — Gap Analysis & Implementation Plan

Godot-style game editor for Spark: **2D and 3D**, built on the engine’s retained UI (`spark/ui/`), delivered as a **separate executable** linking **`SparkEngine` as a shared library**.

**Related:** [`GUI_EDITOR_ROADMAP.md`](GUI_EDITOR_ROADMAP.md) (GUI milestones E0–E6), [`ARCHITECTURE_AND_DEVELOPER_GUIDE.md`](ARCHITECTURE_AND_DEVELOPER_GUIDE.md), [`SCENE_AND_RENDERING_GAPS.md`](SCENE_AND_RENDERING_GAPS.md), demo **#12** `SceneEditor3DDemo` (prototype).

---

## 1. Executive summary

| Layer | Status | Editor impact |
|-------|--------|---------------|
| Engine loop / ECS / `GameWorld` | **Ready** | Core simulation model |
| Vulkan forward renderer | **Ready** | Viewport scissor for center pane; offscreen RT later |
| UI toolkit | **Usable** | Docking (`SparkDockWorkspace` / `IDockWorkspace`), panels, context menu, text overflow |
| Scene serialization v4 | **Partial** | **23** component handlers; text format (reads v3) |
| `SceneEditor3DDemo` | **Prototype** | Pick, translate gizmo, save/load — reference for viewport service |
| **`SparkEditor` (M0)** | **Started** | `EditorApplication`, dock shell, hierarchy/inspector stubs, fly camera |
| Project / asset DB | **Missing** | Critical for Godot workflow |
| Inspector property grid | **Missing** | Critical |
| Undo / PIE | **Missing** | Critical for v1 |

**Strategy:** `spark/editor/` services inside the engine repo; ship **`spark_editor/SparkEditor`** via root CMake (`SPARK_BUILD_SPARK_EDITOR=ON`) or slim **`editor-debug`** preset (shared `SparkEngine`).

---

## 2. What exists (reusable)

### Engine & ECS
- `IGame` / `Game` / `Engine::Run` — editor is an `IGame` implementation (`spark_editor/EditorGame`).
- `GameWorld`, `GameObject`, **37** `ComponentKind` values (+ `Unknown`), hierarchy via `SetParent`.
- `Scene` + `SubmitStandardLitSceneFromWorld` — no Vulkan in editor code.
- `FlyCamera`, `MeshRaycast`, lighting profiles, 2D/3D cameras.

### UI
- Full control set via `SparkUiControlsFactory`: `TreeView`, panels, scroll regions, sliders, lists.
- **Docking:** `SparkDockWorkspace` / `IDockWorkspace` — used by `EditorDockShell`.
- `UiCanvasComponent` + `UiConsumesGamePointer()` for viewport input gating.
- `UiContextMenu`, text overflow via `UiPaintContext`.
- `EditorLayoutStore` — persists `editor_layout.ini`.

### Scene authoring prototype (`SceneEditor3DDemo`)
- Translate gizmo, ray pick, context menu place/delete, `spark_scene_v4` save/load (reads v3).
- **Port target:** `EditorViewport` service (M3), not rewrite.

### Serialization
- `SceneSerializer` / `ComponentSnapshotRegistry` — **23** handlers: Transform, Mesh, Material, DirectionalLight, PointLight, SpotLight, Camera, SkinnedMesh, Animator, Sky, Sprite, SceneSpatialPolicy, TextOverlay, ParticleEmitter, Terrain, BoxCollider3D, SphereCollider3D, Rigidbody3D, PhysicsMaterial3D, RenderLayer, SortingGroup, Camera2D, Camera2DRig. Runtime: `SceneManager`, `GameWorldAssetLoader`.
- Extensible handler pattern for remaining kinds (`UiCanvas`, `Tilemap`, 2D physics, `AiAgent`, …).

### Build
- `SPARK_BUILD_ENGINE_SHARED` — proven in `game_template/`, `samples/*`.
- `SPARK_BUILD_ENGINE_DEMOS` — **new**; omit demos from slim DLL.
- `SPARK_BUILD_EDITOR` — **new**; compiles `spark/editor/` into `SparkEngine`.

### Scripting (future PIE)
- `SparkScriptHost`, `SparkInterop`, C# `HelloCsGame`.

---

## 3. Missing engine capabilities (by priority)

### 3.1 Critical (blocks Godot-like editor)

| Gap | Why it matters | Milestone |
|-----|----------------|-----------|
| **Editor product module** | `spark/editor/` services (M0 shell) | **M0** ✓ started |
| **Demo/engine split** | `SPARK_BUILD_ENGINE_DEMOS` CMake option | **M0** ✓ |
| **`EditorApplication` shell** | Edit mode, dock UI, fly camera | **M0** ✓ |
| **Project file (`project.spark`)** | Open folder, main scene, asset roots | **M1** |
| **Asset database** | GUIDs, import, thumbnails — not just runtime cache | **M1–M2** |
| **Hierarchy panel** | `HierarchyPanel` + `TreeView` stub; CRUD / reparent pending | **M2** (started) |
| **Inspector / property grid** | `InspectorPanel` text dump of selection | **M2–M3** (started) |
| **`EditorSelection` service** | Primary selection wired to inspector | **M2** ✓ |
| **Dock workspace** | `EditorDockShell` + `SparkDockWorkspace` (menu, left/center/right) | **M0** ✓ |
| **Embedded viewport service** | `worldViewportScissor` + passthrough center pane | **M2** (partial) |
| **Undo/redo command stack** | None | **M3** |
| **Serialization coverage** | 23 / 37 components; no `UiCanvas`, `Tilemap`, 2D physics, `AiAgent`, … | **M3** |
| **Play-in-editor (PIE)** | No world snapshot / script run from editor | **M5** |
| **Prefab / `.sparkscene` asset** | Loose `scene.txt` only | **M3** |

### 3.2 Important (v1 quality)

| Gap | Milestone |
|-----|-----------|
| Rotate/scale gizmos | M3 |
| Selection outline pass (not emissive pulse) | M3 |
| GPU pick buffer | M4 |
| Add/remove component UI | M3 |
| Material slot editor | M4 |
| Animation timeline | M4 (needs ANIMATION_3D) |
| Script attach + build console | M5 |
| Physics collider debug draw | M4 |
| Offscreen “Game” camera view | M5 |
| Global shortcuts (Save, Undo, Play) | M2–M3 |
| Export / build pipeline | M6 |

### 3.3 Nice-to-have (Godot parity)

Material graph, visual scripting, plugins, tilemap/terrain brushes, LSP, CI scene round-trip — see [`GUI_EDITOR_ROADMAP.md`](GUI_EDITOR_ROADMAP.md) E6+.

---

## 4. Architecture

```
┌─────────────────────────────────────────────────────────┐
│  spark_editor/SparkEditor.exe  (IGame: EditorGame)       │
├─────────────────────────────────────────────────────────┤
│  spark/editor/panels   Hierarchy, Inspector, Project, …  │
├─────────────────────────────────────────────────────────┤
│  spark/editor services EditorApplication, Selection,     │
│                          Project, CommandStack (future)  │
├─────────────────────────────────────────────────────────┤
│  spark/ui              retained IUiElement tree + themes │
├─────────────────────────────────────────────────────────┤
│  SparkEngine.dylib     ECS, scene, render, audio, …      │
└─────────────────────────────────────────────────────────┘
```

### OOP principles
- **`IEditorPanel`** — single responsibility per dock panel; register with `EditorApplication`.
- **`EditorContext`** — façade passed to panels (world, selection, project); avoids globals.
- **`EditorSelection`** — owns selection set; panels subscribe via callbacks.
- **`EditorProject`** — project path, settings, dirty flags; no rendering knowledge.
- **Extensibility** — new panels implement `IEditorPanel`; new components add `IComponentSnapshotHandler` + inspector widget.

### 2D vs 3D
- Same shell: **2D** uses `Camera2D` + sprite/tilemap components; **3D** uses `FlyCamera` / `CameraComponent` + meshes.
- `EditorApplication::SetWorkspaceMode(Mode2D | Mode3D)` switches default scene template and viewport controller (M2).

---

## 5. Roadmap & task tracking

| ID | Milestone | Goal | Exit criteria |
|----|-----------|------|---------------|
| **M0** | Foundation | Slim DLL + editor exe | `SparkEditor` runs, empty 3D scene + dock shell |
| **M1** | E0 + shell | Menu, project browser stub, layout | Open folder, persist panel widths |
| **M2** | E1–E2 start | Hierarchy + selection + viewport contract | Select entity in tree ↔ viewport highlight |
| **M3** | E2 core | Scene save v4, undo, gizmo port | Author `.sparkscene` in project folder |
| **M4** | E3–E4 | Materials, animation basics | Texture slots, clip scrubber |
| **M5** | E5 | PIE + C# scripts | Play stops, restores edit world |
| **M6** | E6 | Product | New project wizard, user guide, CI round-trip |

### M0 — Foundation (**complete**)

- [x] `SPARK_BUILD_ENGINE_DEMOS`, `SPARK_BUILD_EDITOR` CMake options
- [x] `spark/editor/` module: `EditorApplication`, `EditorSelection`, `EditorProject`
- [x] `spark_editor/` CMake project → `SparkEditor` + shared `SparkEngine`
- [x] Dock shell: menu bar + hierarchy + inspector + project stubs
- [x] Default 3D scene (ground + sun + sample cube), fly camera viewport
- [x] `docs/SPARK_EDITOR_PLAN.md` (this file)
- [x] Build verified: `SparkEngine` + `SparkEditor` compile

### M1 — Editor shell (next)

- [ ] `project.spark` JSON (name, `assets/`, `scenes/main.sparkscene`)
- [ ] `ProjectBrowserPanel` — filesystem `TreeView`
- [ ] `EditorLayoutStore` integration for dock widths
- [ ] File menu: New/Open/Save project

### M2 — Scene tools

- [ ] `HierarchyPanel` ↔ `GameWorld` CRUD
- [ ] `EditorSelection` multi-select
- [ ] Viewport sub-rect + `UiConsumesGamePointer` for chrome only
- [ ] Port pick/gizmo from `SceneEditor3DDemo`

### M3 — Authoring

- [ ] `EditorCommandStack` (move, create, delete, property set)
- [ ] Scene format v4 (JSON) + more component handlers
- [ ] Inspector for Transform, Mesh, Material, lights

---

## 6. Project layout

```text
spark/                          # engine repository
├── include/spark/editor/       # public editor API (linked via SparkEngine)
├── src/spark/editor/
├── spark_editor/               # NEW — editor executable project
│   ├── CMakeLists.txt
│   └── src/EditorGame.cpp
├── game_template/              # game DLL pattern (reference)
└── docs/SPARK_EDITOR_PLAN.md

```

### Build editor

```bash
cmake -S spark/spark_editor -B build-editor \
  -DSPARK_ROOT=../
cmake --build build-editor --target SparkEditor
```

---

## 7. Godot comparison (target v1)

| Godot | Spark Editor M6 target |
|-------|------------------------|
| Scene tree dock | Hierarchy panel |
| Inspector | Inspector panel |
| 3D viewport + gizmo | Port from SceneEditor3DDemo |
| FileSystem | Project browser + asset DB |
| `.tscn` | `.sparkscene` (v4 JSON) |
| Play (F5) | PIE world snapshot |
| Script editor | TextArea + `dotnet build` (M5) |
| Undo | Command stack (M3) |

---

## 8. Immediate next steps

1. **M1** — `project.spark` JSON + open-folder dialog; wire File menu.
2. **M2** — Hierarchy CRUD, viewport sub-rect, port pick/gizmo from `SceneEditor3DDemo`.
3. **M3** — `EditorCommandStack`, scene v4 JSON, richer inspector.
4. Add **component handlers** incrementally — each unlocks inspector + save.

Update this document at each milestone; cross-link task IDs with [`GUI_EDITOR_ROADMAP.md`](GUI_EDITOR_ROADMAP.md).
