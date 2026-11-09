# Spark Game Engine — Programming Guide

A comprehensive developer guide for building **2D and 3D games** with Spark — a **C++23** engine using GLFW, Vulkan, ECS-style entities, custom physics, AI modules, retained-mode GUI, and optional **Dear ImGui** tool UI.

## Who This Guide Is For

- C++ gameplay programmers and engine contributors
- Teams evaluating Spark for desktop games
- Developers migrating from Unity/Unreal to a code-first workflow

## Prerequisites

- C++23, CMake 3.28+, Vulkan SDK
- Vectors, matrices, basic rendering concepts
- Spark repository cloned locally

## Ten Parts

| Part | Folder | Focus |
|------|--------|-------|
| **1** | `1-overview-architecture/` | Engine loop, ECS, component reference, scenes, assets, catalog, scripting, debugging, arcade, **roadmap hub** |
| **2** | `2-2d-graphics/` | Sprites, cameras, tilemaps, **level authoring**, render layers, 2D pipeline |
| **3** | `3-3d-graphics/` | Meshes, PBR, lighting, skinning, terrain, VFX, **environment stack**, **fog/post volumes** |
| **4** | `4-ai/` | FSM, GOAP, pathfinding, steering, **perception & nav** |
| **5** | `5-physics/` | 2D/3D solvers, colliders, queries, **layers**, **triggers/contacts** |
| **6** | `6-sound/` | Mixer, clips, cues, **spatial audio**, **ambient zones** |
| **7** | `7-game-components-usage/` | **Game components usage** — grouped guides with C++ / C\# / Python per component family |
| **8** | `8-2d-game/` | **2D game (unified)** — platformer, tilemap gameplay, nav, minimap, P0 path, **2D handbook** |
| **9** | `9-3d-game/` | **3D game** — FPS, character camera, physics showcase, maze AI, **3D handbook** |
| **10** | `10-tools-and-workflows/` | Scene editor, ImGui tools, Spark editor direction |

### Part 1 — new hub chapters

| Ch. | Topic |
|-----|--------|
| [10](1-overview-architecture/10-sparkdemo-catalog.md) | SparkDemo catalog & learning paths |
| [11](1-overview-architecture/11-scenes-and-saves.md) | `.sparkscene`, pools, `GameSave` |
| [12](1-overview-architecture/12-assets-and-content-pipeline.md) | Meshes, glTF pump, asset roots |
| [13](1-overview-architecture/13-scripting-and-bindings.md) | C\# / Python interop boundaries |
| [14](1-overview-architecture/14-debugging-and-diagnostics.md) | Help HUD, validation, tick order, perf |
| [15](1-overview-architecture/15-arcade-single-file-games.md) | Tetris / Match-3 / Space Invaders (#11–13) |
| [16](1-overview-architecture/16-glossary-and-roadmap-hub.md) | Glossary + links to `docs/*.md` roadmaps |

## Repository Map

| Path | Role |
|------|------|
| `include/spark/` | Public API |
| `src/spark/demo/Platformer2DDemo.cpp` | 2D platformer reference (SparkDemo **#6**) |
| `docs/programming-guide/7-game-components-usage/` | **Part 7** component usage chapters |
| `docs/programming-guide/1-overview-architecture/07-game-component-reference.md` | Full **120** `GameComponent` API tables |
| `docs/programming-guide/8-2d-game/09-2d-gameplay-api-guide.md` | Tilemap + nav + minimap workflow |
| `docs/programming-guide/8-2d-game/10-2d-components-handbook.md` | Per-kind handbook (2D angle) |
| `docs/programming-guide/9-3d-game/10-3d-components-handbook.md` | 3D component stacks + checklists |
| `docs/programming-guide/10-tools-and-workflows/` | In-engine tools (demos #10, #20, #24) |

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
./build/SparkDemo
```

## Contributor & roadmap docs

Full link table: [Glossary & roadmap hub](1-overview-architecture/16-glossary-and-roadmap-hub.md). Highlights:

| Document | When to read |
|----------|----------------|
| [ARCHITECTURE_AND_DEVELOPER_GUIDE.md](../ARCHITECTURE_AND_DEVELOPER_GUIDE.md) | Engine contributors, Vulkan/subsystem deep dive |
| [SPARK_EDITOR_PLAN.md](../SPARK_EDITOR_PLAN.md) | Editor product direction — [Part 10](10-tools-and-workflows/00-tools-and-workflows-map.md) |

Start with [Introduction](1-overview-architecture/01-introduction.md). Pick a demo: [SparkDemo catalog](1-overview-architecture/10-sparkdemo-catalog.md). For component examples: [Part 7](7-game-components-usage/00-introduction.md). For 2D / 3D games: [Part 8](8-2d-game/00-2d-game-systems-map.md) / [Part 9](9-3d-game/00-3d-game-systems-map.md). Tools: [Part 10](10-tools-and-workflows/00-tools-and-workflows-map.md).
