---
title: Spark Game Engine Programming Guide
order: 0
---

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

## Eight Parts (47 Chapters)

| Part | Folder | Focus |
|------|--------|-------|
| **1** | `1-overview-architecture/` | Engine loop, interfaces, ECS, **component reference**, **UI toolkits**, render contract |
| **2** | `2-2d-graphics/` | Sprites, cameras, tilemaps, 2D pipeline |
| **3** | `3-3d-graphics/` | Meshes, PBR, lighting, skinning, terrain |
| **4** | `4-ai/` | Blackboard, FSM, GOAP, pathfinding, steering |
| **5** | `5-physics/` | 2D/3D solvers, colliders, queries, layers |
| **6** | `6-sound/` | Mixer, clips, cues, background music |
| **7** | `7-2d-game/` | Full platformer walkthrough |
| **8** | `8-3d-game/` | Full FPS arena walkthrough |

## Repository Map

| Path | Role |
|------|------|
| `include/spark/` | Public API |
| `src/spark/` | Implementations |
| `samples/platformer2d_game_template/` | 2D game sample |
| `samples/fps_game_template/` | 3D FPS sample |
| `docs/ARCHITECTURE_AND_DEVELOPER_GUIDE.md` | Contributor deep-dive |
| `docs/programming-guide/1-overview-architecture/07-game-component-reference.md` | All 64 `GameComponent` types + examples |

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
./build/SparkDemo
```

Start with [Introduction](overview-architecture/01-introduction.html).
