# Scene editor 3D (demo #10)

`SceneEditor3DDemo` is the in-engine **placement and lighting** workspace: fly/orbit camera, transform gizmo, asset browser, glTF import, **`.sparkscene` save/load**, and a minimal play-in-editor loop.

**Source:** `src/spark/demo/SceneEditor3DDemo.cpp` · **Scenes:** [Part 1 ch.11](../1-overview-architecture/11-scenes-and-saves.md).

## What you can do today

| Feature | API / component |
|---------|-----------------|
| Select & move props | Transform gizmo (W/E/R), `contentModel` selection |
| Pick in viewport | `SceneRaycast` / mesh raycast |
| Place primitives | Unit cube mesh, registered assets |
| Directional sun | `DirectionalLightComponent` template object |
| Asset panel | `assetBrowser` + `assetCatalog` refresh |
| Context menu (RMB) | `UiContextMenu` + `UiCanvasComponent` |
| Serialize | `SceneSerializer`, `SceneManager`, `SceneLoadSession` |
| Import glTF | `PrefabInstantiator`, `GltfAssetBindings` |

## Controls (from help HUD)

**W / E / R** gizmo translate/rotate/scale · **F** focus selection · **Home** reset camera · **asset panel** left · **RMB** context menu · **I** import · **P** play preview · **Esc** stop play

## Concept: editor scene root pattern

#### C++

```cpp
#include "spark/demo/DemoHelpHud.hpp"
#include "spark/ui/runtime/UiScene.hpp"

void SceneEditor3DDemo::Load(GameWorld& w, IEngineContext& context) {
    contentModel.ClearLists();
    unitCubeAsset = MakeShared<Mesh>(Utf8String("UnitCube"));
    *unitCubeAsset = Mesh::CreateUnitCube();
    w.RegisterMesh(unitCubeAsset, "spark/scene_editor/unit_cube");

    GameObject* ground = w.CreateGameObject("SceneEditorGround");
    ground->AddComponent<TransformComponent>();
    ground->AddComponent<MeshComponent>(groundAsset, SceneMeshSlot::GroundPlane, tint);
    contentModel.TrackRoot(ground);

    SetupContextMenuCanvas(w);
    assetBrowser.Mount(w, assetCatalog, *this);

    helpHud.Mount(w, "Scene editor");
    sceneManager = MakeUnique<SceneManager>(w);
}
```

Track placed objects in a **`DemoRootCollection`** or editor-specific registry so `Unload` destroys only editor-owned roots.

#### C\#

```csharp
// Scene editor is C++-hosted; managed tools call the same SceneSerializer APIs via bindings when exposed.
```

#### Python

```python
pass
```

## Concept: save after edit

#### C++

```cpp
#include "spark/scene/serialization/SceneSerializer.hpp"

SceneDocument doc{};
SceneSerializer::BuildFromWorld(world, doc);
SceneSerializer::WriteToFile(doc, pathUtf8);
```

Round-trip constraints: only components with snapshot handlers persist — see [Scenes and saves](../1-overview-architecture/11-scenes-and-saves.md).

#### C\#

```csharp
// SceneSerializer when bound in spark-sharp host.
```

#### Python

```python
pass
```

## Concept: retained UI in tools

The scene editor uses **`spark/ui/`** (panels, browser, context menu) — not immediate ImGui — so layout can eventually serialize with scenes. Compare with [Dear ImGui showcase](02-dear-imgui-and-tool-ui.md).

## Integration checklist

| Step | Action |
|------|--------|
| 1 | Run SparkDemo → demo **#10** |
| 2 | Place lights/meshes; verify pick + gizmo |
| 3 | Save `.sparkscene`; reload in a minimal `Game` loader |
| 4 | Use **P** play mode to validate transforms without leaving editor |
| 5 | For shipping, strip editor-only `UiCanvas` from player builds |

## Limitations (expectation setting)

- No terrain sculpt brush in editor mode (use **#4** Terrain demo).  
- 2D tilemap painting: [Tilemap level authoring](../2-2d-graphics/08-tilemap-level-authoring.md), not this demo.  
- Multi-user / undo stack: see [`SPARK_EDITOR_PLAN.md`](../../SPARK_EDITOR_PLAN.md).

Next: [Dear ImGui and tool UI](02-dear-imgui-and-tool-ui.md).
