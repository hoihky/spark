# SparkDemo catalog and learning paths

`./build/SparkDemo` (default `src/main.cpp` → `SparkShellDemo`) is an interactive launcher over **30+** built-in samples. Source of truth: `src/spark/demo/DemoCatalog.cpp`. Rows marked **\*** are recommended starting points.

## Launcher controls

| Input | Action |
|-------|--------|
| **1–0** | Jump to demos **1–10** (digit hotkeys where configured) |
| **Letter keys** | Quick open (e.g. **F** = P0 2D path, **P** = Physics 3D, **T** = Tetris) |
| **TAB / arrows** | Change row in the shell list |
| **Enter** | Run highlighted demo |
| In-demo **TAB / H / F3** | Help, hotkey overlay (per demo) |

## Full catalog

| # | Hot | Title | Category | Guide path |
|---|-----|-------|----------|------------|
| 1 | | Basic 3D | 3D Graphics | [Meshes](../3-3d-graphics/01-meshes-and-materials.md), [Part 9](../9-3d-game/00-3d-game-systems-map.md) |
| 2 | | Sky | 3D Graphics | [Terrain & sky](../3-3d-graphics/06-terrain-and-sky.md) |
| 3 | | VFX Showcase | 3D Graphics | [VFX](../3-3d-graphics/08-vfx-and-sparkvfx.md) |
| 4 | | Terrain | 3D Graphics | [Terrain & sky](../3-3d-graphics/06-terrain-and-sky.md) |
| 5 * | | Character camera | 3D Gameplay | [Character & third person](../9-3d-game/07-character-and-third-person.md) |
| 6 * | | 2D platformer | 2D Gameplay | [Part 8 — platformer](../8-2d-game/01-platformer-intro.md), [Spatial audio](../6-sound/04-spatial-audio-and-listeners.md) |
| 7 | | Broad-phase 2D | 2D Gameplay | [Layers & triggers](../5-physics/08-collision-layers-and-filters.md), [joints](../5-physics/09-triggers-contacts-and-showcase.md) |
| 8 | | 3D maze | 3D Gameplay | [Perception & nav](../4-ai/06-perception-nav-and-blackboard.md), [Maze AI](../9-3d-game/09-maze-ai-and-perception.md) |
| 9 | | 3D physics | 3D Gameplay | [FPS setup](../9-3d-game/02-project-setup.md), ball throw |
| 10 | | Scene editor | Tools | [Scene editor 3D](../10-tools-and-workflows/01-scene-editor-3d.md) |
| 11 | T | Tetris | Arcade | [Arcade guide](15-arcade-single-file-games.md#mini-part-a--tetris-11--t) |
| 12 | C | Match-3 | Arcade | [Arcade guide](15-arcade-single-file-games.md#mini-part-b--match-3-12--c) |
| 13 | I | Space Invaders | Arcade | [Arcade guide](15-arcade-single-file-games.md#mini-part-c--space-invaders-13--i) |
| 14 | ; | 3D steering | 3D Gameplay | [Steering](../4-ai/02-steering.md), [Perception & nav](../4-ai/06-perception-nav-and-blackboard.md) |
| 15 | Y | Toon / cel | 3D Graphics | [Lighting](../3-3d-graphics/03-lighting.md) (toon shading model) |
| 16 | B | Material ball | 3D Graphics | [Materials](../3-3d-graphics/01-meshes-and-materials.md) |
| 17 * | N | Time of day | 3D Graphics | [Post-process & volumes](../3-3d-graphics/10-post-process-fog-volumes.md), [Shadows](../3-3d-graphics/04-shadows.md) |
| 18 | | 2D render layers | 2D Gameplay | [Render layers](../2-2d-graphics/07-render-layers-and-sorting.md) |
| 19 | | Tilemap showcase | 2D Gameplay | [Tilemap level authoring](../2-2d-graphics/08-tilemap-level-authoring.md), [Part 8 API](../8-2d-game/09-2d-gameplay-api-guide.md) |
| 20 | G | Dear ImGui tools | Tools | [ImGui showcase](../10-tools-and-workflows/02-dear-imgui-and-tool-ui.md) |
| 21 * | Q | glTF PBR showcase | 3D Graphics | [Skinned / glTF](../3-3d-graphics/05-skinned-characters.md), [Assets](12-assets-and-content-pipeline.md) |
| 22 | V | 3D model viewer | Tools | Asset inspection |
| 23 * | W | Water lake | 3D Graphics | [Environment stack](../3-3d-graphics/09-environment-stack.md) |
| 24 | E | Spark editor | Tools | [Editor direction](../10-tools-and-workflows/03-spark-editor-direction.md) |
| 25 | X | 2D VFX showcase | 2D Gameplay | [VFX](../3-3d-graphics/08-vfx-and-sparkvfx.md) |
| 26 * | F | 2D P0 product path | 2D Gameplay | [P0 path](../8-2d-game/07-p0-2d-product-path.md), [Scenes](11-scenes-and-saves.md) |
| 27 * | G | Foliage wind meadow | 3D Graphics | [Environment stack](../3-3d-graphics/09-environment-stack.md) |
| 28 * | H | Grass field terrain | 3D Graphics | [Environment stack](../3-3d-graphics/09-environment-stack.md) |
| 29 * | P | Physics 3D showcase | 3D Gameplay | [Layers & showcase](../5-physics/09-triggers-contacts-and-showcase.md), [Physics product path](../9-3d-game/08-physics-3d-product-path.md) |
| 30 | Z | Spark-pix atlas | 2D Gameplay | Custom sprite atlases |

\* = recommended in launcher (`DemoCatalogEntry::recommended`).

## Suggested learning paths

### New to Spark (C++)

1. [Introduction](01-introduction.md) → [Building and running](03-building-and-running.md)  
2. SparkDemo **#6** + [Part 8](../8-2d-game/00-2d-game-systems-map.md)  
3. [Part 7](../7-game-components-usage/00-introduction.md) while reading [component reference](07-game-component-reference.md)  
4. SparkDemo **#1** or **#5** + [Part 9](../9-3d-game/00-3d-game-systems-map.md)

### Arcade “one module” games

1. [Arcade single-file games](15-arcade-single-file-games.md) overview  
2. Run **#11 (T)**, **#12 (C)**, **#13 (I)** — compare `Load`/`Simulate`/`Render` in each `.cpp`  
3. Graduate to **#6** platformer or [Debugging](14-debugging-and-diagnostics.md) when adding scenes

### C\# / Python gameplay

1. [Scripting and bindings](13-scripting-and-bindings.md)  
2. Part 7 chapters (tri-language examples)  
3. SparkDemo **#26 (F)** with `spark-sharp` host for P0 flow

### Tools & authoring

1. [Part 10 map](../10-tools-and-workflows/00-tools-and-workflows-map.md)  
2. Demo **#10** scene editor · **#20** ImGui · **#24** Spark editor  
3. [Glossary & roadmaps](16-glossary-and-roadmap-hub.md) for editor plans

### Tilemap / authored levels

1. SparkDemo **#19**  
2. [Scenes and saves](11-scenes-and-saves.md)  
3. [2D gameplay API](../8-2d-game/09-2d-gameplay-api-guide.md)

### Physics-heavy 3D

1. SparkDemo **#29 (P)**  
2. [Physics 3D product path](../9-3d-game/08-physics-3d-product-path.md)  
3. [Part 5](../5-physics/05-physics-3d.md)

Next: [Scenes and saves](11-scenes-and-saves.md).
