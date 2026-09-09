# Spark documentation index

**Build / preview HTML:** from this folder run `./build-all.sh`, then `python3 -m http.server 8080` and open `http://127.0.0.1:8080/index.html`. Generated pages embed theme CSS inline so they also work when opened directly as local files.

| Document | Audience | Summary |
|----------|----------|---------|
| [**../README.md**](../README.md) | Everyone | Quick start, build targets, repo map |
| [**programming-guide/**](programming-guide/index.md) | Game devs | Multi-part tutorial (tilemaps, TMX import, gameplay grid, pathfinding), [**UI toolkits**](programming-guide/1-overview-architecture/08-ui-and-toolkits.md), [**component reference**](programming-guide/1-overview-architecture/07-game-component-reference.md) |
| [**GLTF_DISPLAY_ROADMAP.md**](GLTF_DISPLAY_ROADMAP.md) | Rendering / assets | glTF display compatibility phases and test matrix |
| [**ARCHITECTURE_AND_DEVELOPER_GUIDE.md**](ARCHITECTURE_AND_DEVELOPER_GUIDE.md) | Engine & gameplay devs | Loop, ECS, rendering, feature catalog |
| [**SCENE_AND_RENDERING_GAPS.md**](SCENE_AND_RENDERING_GAPS.md) | Gameplay devs | C++ public API gaps — scene, ECS, rendering (`include/spark/`) |
| [**CLION.md**](CLION.md) | IDE users | CMake presets, editor-debug profile |
| [**SPARK_EDITOR_PLAN.md**](SPARK_EDITOR_PLAN.md) | Editor contributors | Godot-style editor milestones M0–M6; scene save/load, `project.spark`, glTF prefab overrides |
| [**VFX_ROADMAP.md**](VFX_ROADMAP.md) | VFX / gameplay | Particle system phases P1–P5, built-in effects, `VfxLibrary` |
| [**GUI_EDITOR_ROADMAP.md**](GUI_EDITOR_ROADMAP.md) | UI / tools devs | `spark/ui` control inventory, editor UX tasks |
| [**LIGHTING_AND_SHADOWS.md**](LIGHTING_AND_SHADOWS.md) | Rendering | CSM, punctual lights, SSAO, frame order |
| [**MATERIALS_AND_LIGHTING.md**](MATERIALS_AND_LIGHTING.md) | Artists / rendering | PBR channels, IBL, material limits |
| [**ANIMATION_3D_ROADMAP.md**](ANIMATION_3D_ROADMAP.md) | Animation | Skeletal animation milestones |
| [**ANIMATION_SAMPLE_ASSETS.md**](ANIMATION_SAMPLE_ASSETS.md) | Animation | Fox / CesiumMan clip tables, FSM resolve, test matrix |
| [**BLENDER_GLTF_ANIMATION_EXPORT.md**](BLENDER_GLTF_ANIMATION_EXPORT.md) | Artists | Blender export checklist for skinned glTF |
| [**CSHARP_SCRIPTING.md**](CSHARP_SCRIPTING.md) | Scripting | CoreCLR host, C# bindings |
| [**OPEN_WORLD_ACTION_ROADMAP.md**](OPEN_WORLD_ACTION_ROADMAP.md) | Long-term | Streaming, combat, AI phases |
| [**2D_ARPG_FEATURES.md**](2D_ARPG_FEATURES.md) | 2D gameplay | Physics, queries, anim FSM backlog |

**Also:** [`.run/README.md`](../.run/README.md) (CLion run configs), [`spark_editor/CLION.md`](../spark_editor/CLION.md), [`assets/CREDITS.md`](../assets/CREDITS.md).
