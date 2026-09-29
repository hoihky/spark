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

## Eight Parts (50+ Chapters)

| Part | Folder | Focus |
|------|--------|-------|
| **1** | `1-overview-architecture/` | Engine loop, interfaces, ECS, **component reference**, **UI toolkits**, render contract |
| **2** | `2-2d-graphics/` | Sprites, cameras, tilemaps, 2D render pipeline |
| **3** | `3-3d-graphics/` | Meshes, PBR, lighting, skinning, terrain |
| **4** | `4-ai/` | Blackboard, FSM, GOAP, pathfinding, steering |
| **5** | `5-physics/` | 2D/3D solvers, colliders, queries, layers |
| **6** | `6-sound/` | Mixer, clips, cues, background music |
| **7** | `7-2d-game/` | **2D game (unified)** — platformer walkthrough, tilemap gameplay, nav, minimap, P0 path, [components handbook](7-2d-game/10-2d-components-handbook.md) |
| **8** | `8-3d-game/` | Full FPS arena walkthrough |

## Repository Map

| Path | Role |
|------|------|
| `include/spark/` | Public API |
| `src/spark/` | Implementations |
| `src/spark/demo/Platformer2DDemo.cpp` | 2D platformer reference (SparkDemo **#6**) |
| `src/spark/demo/GameFlow2DProductPathDemo.cpp` | P0 grid ARPG path (SparkDemo **#26 / F**) |
| `docs/programming-guide/7-2d-game/09-2d-gameplay-api-guide.md` | Tilemap + nav + minimap workflow |
| `docs/programming-guide/7-2d-game/10-2d-components-handbook.md` | All components with 2D examples |
| `docs/programming-guide/1-overview-architecture/07-game-component-reference.md` | Full **108** `GameComponent` types (by section) |

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
./build/SparkDemo
```

Start with [Introduction](1-overview-architecture/01-introduction.md). For 2D games, open [Part 7 overview](7-2d-game/00-2d-game-systems-map.md).
