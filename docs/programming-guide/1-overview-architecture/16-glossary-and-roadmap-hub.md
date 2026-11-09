# Glossary and roadmap hub

The programming guide teaches **how to build games**. Sibling files under `docs/` track **engine plans**, binding detail, and contributor Vulkan notes. Use this page to pick the right doc without duplicating roadmaps inside the guide.

**Contributor depth:** [`ARCHITECTURE_AND_DEVELOPER_GUIDE.md`](../../ARCHITECTURE_AND_DEVELOPER_GUIDE.md) · **All demos:** [SparkDemo catalog](10-sparkdemo-catalog.md).

## Glossary (in-guide terms)

| Term | Meaning |
|------|---------|
| `GameWorld` | ECS container; owns `GameObject` graph |
| `Scene` / `SceneRenderParams` | Render-facing description built each frame |
| `SubmitStandardLitSceneFromWorld` | Main 3D collect + draw bridge |
| `PhysicsSubsystem` | You must call `Simulate*` explicitly |
| `SimulateGameAi` | AI/nav/perception batch — not automatic |
| `ComponentKind` | Stable enum for every `GameComponent` |
| `.sparkscene` | Serialized level snapshot (v4) |
| `.sparkmap` / TMX | Tilemap document interchange |
| `.sparkvfx` | Serialized VFX effect definition |
| `UiCanvas` | Retained UI root (`spark/ui/`) |
| `FlyCamera` | Demo pattern; not an ECS component |

## Roadmap and gap documents

Read these when the guide says “planned” or when you hit scale limits.

### Rendering, materials, world

| Document | When you need it |
|----------|------------------|
| [`SCENE_AND_RENDERING_GAPS.md`](../../SCENE_AND_RENDERING_GAPS.md) | Multi-scene streaming, render features not in public API |
| [`MATERIALS_AND_LIGHTING.md`](../../MATERIALS_AND_LIGHTING.md) | PBR limits, texture array budget |
| [`LIGHTING_AND_SHADOWS.md`](../../LIGHTING_AND_SHADOWS.md) | CSM, punctual caps, IBL detail |
| [`GLTF_DISPLAY_ROADMAP.md`](../../GLTF_DISPLAY_ROADMAP.md) | glTF import/display evolution |
| [`VFX_ROADMAP.md`](../../VFX_ROADMAP.md) | `.sparkvfx` editor + GPU effect backlog |
| [`WATER_ROADMAP.md`](../../WATER_ROADMAP.md) | Ocean/lake feature depth |
| [`FOLIAGE_ROADMAP.md`](../../FOLIAGE_ROADMAP.md) | Grass/instancing scale |

### Animation and 3D action

| Document | When you need it |
|----------|------------------|
| [`ANIMATION_3D_ROADMAP.md`](../../ANIMATION_3D_ROADMAP.md) | Locomotion layers, IK, combat anim milestones |
| [`ANIMATION_SAMPLE_ASSETS.md`](../../ANIMATION_SAMPLE_ASSETS.md) | Test characters and export validation |
| [`BLENDER_GLTF_ANIMATION_EXPORT.md`](../../BLENDER_GLTF_ANIMATION_EXPORT.md) | Artist export rules |
| [`3D_ACTION_GAME_GAPS.md`](../../3D_ACTION_GAME_GAPS.md) | Shipping action title gap list (skim with roadmaps) |
| [`OPEN_WORLD_ACTION_ROADMAP.md`](../../OPEN_WORLD_ACTION_ROADMAP.md) | Squads, streaming, crowds, long-horizon |

### Tools, UI, 2D product

| Document | When you need it |
|----------|------------------|
| [`SPARK_EDITOR_PLAN.md`](../../SPARK_EDITOR_PLAN.md) | In-engine editor product direction |
| [`GUI_EDITOR_ROADMAP.md`](../../GUI_EDITOR_ROADMAP.md) | Editor UI panels |
| [`GUI_TOOLKIT_ARCHITECTURE.md`](../../GUI_TOOLKIT_ARCHITECTURE.md) | Retained vs ImGui |
| [`2D_ARPG_FEATURES.md`](../../2D_ARPG_FEATURES.md) | 2D combat/layers feature checklist |

### Scripting and build

| Document | When you need it |
|----------|------------------|
| [`CSHARP_SCRIPTING.md`](../../CSHARP_SCRIPTING.md) · [`SPARK_SHARP.md`](../../SPARK_SHARP.md) | Managed host |
| [`COMPONENT_SCRIPT_CODEGEN.md`](../../COMPONENT_SCRIPT_CODEGEN.md) | Regenerating bindings |
| [`BINDINGS_COVERAGE.md`](../../BINDINGS_COVERAGE.md) | C\# / Python surface audit |
| [`CLION.md`](../../CLION.md) | IDE setup for contributors |

## Guide parts ↔ roadmaps

| Part | Companion roadmaps |
|------|---------------------|
| **2** 2D | `2D_ARPG_FEATURES`, tilemap editor roadmap (in Part 2) |
| **3** 3D | `WATER`, `FOLIAGE`, `VFX`, `MATERIALS_AND_LIGHTING` |
| **4** AI | `OPEN_WORLD_ACTION` phase K |
| **5** Physics | `3D_ACTION_GAME_GAPS` physics rows |
| **8** 2D game | `2D_ARPG_FEATURES`, P0 path chapters |
| **9** 3D game | `ANIMATION_3D`, `OPEN_WORLD_ACTION` |
| **10** Tools | `SPARK_EDITOR_PLAN`, `GUI_EDITOR_ROADMAP` |

## Program index (quick)

| Part | Start here |
|------|------------|
| 1 | [Introduction](01-introduction.md) |
| 2 | [Sprites](../2-2d-graphics/01-sprites.md) |
| 3 | [Meshes](../3-3d-graphics/01-meshes-and-materials.md) |
| 7 | [Component usage](../7-game-components-usage/00-introduction.md) |
| 8 | [2D systems map](../8-2d-game/00-2d-game-systems-map.md) |
| 9 | [3D systems map](../9-3d-game/00-3d-game-systems-map.md) |
| 10 | [Tools map](../10-tools-and-workflows/00-tools-and-workflows-map.md) |

Field tables: [Game component reference](07-game-component-reference.md) · 2D handbook: [ch.10](../8-2d-game/10-2d-components-handbook.md) · 3D handbook: [ch.10](../9-3d-game/10-3d-components-handbook.md).

Next: [Architecture & developer guide](../../ARCHITECTURE_AND_DEVELOPER_GUIDE.md) (external depth).
