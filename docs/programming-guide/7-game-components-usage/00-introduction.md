# Part 7 — Game Components Usage

This part is the **task-oriented companion** to [Part 1 — Game Component Reference](../1-overview-architecture/07-game-component-reference.md). Components are grouped **by function** (rendering, physics, AI, …). Every section includes **C++**, **C\#**, and **Python** examples aligned with `component-script-catalog.json` and `CppMirrors.g.cs`.

## How to read these chapters

| Chapter | Components covered |
|---------|-------------------|
| [2D rendering](01-2d-rendering.md) | Sprite, lighting 2D, parallax, text, particles |
| [3D rendering](02-3d-rendering.md) | Mesh, material, terrain, water, decals |
| [Tilemap](03-tilemap.md) | Map, collider, grid, TMX, objects |
| [Lighting](04-lighting.md) | Directional, point, spot, point 2D |
| [Camera](05-camera.md) | Camera2D rig, 3D camera, spring arm |
| [Physics 2D](06-physics-2d.md) | Bodies, colliders, character controller |
| [Physics 3D](07-physics-3d.md) | Rigidbodies, triggers, joints |
| [Animation](08-animation.md) | Sprite + skeletal animators, events, hitboxes |
| [Input & gameplay](09-input-and-gameplay.md) | Actions, health, pickups, game flow |
| [AI, audio & world](10-ai-audio-world.md) | Nav, sound, streaming, scene policy |

## Prerequisites

- [ECS and Scene](../1-overview-architecture/06-ecs-and-scene.md) — entities, signals, update order  
- [Building and Running](../1-overview-architecture/03-building-and-running.md) — CMake, SparkDemo  
- [Scripting and bindings](../1-overview-architecture/13-scripting-and-bindings.md) — C\# / Python limits and regen  

## Python note

Use `GameObject.add_*` shorthands when spark-python exposes them; otherwise `get_or_add_default_component(ComponentKind.…)` and typed wrappers under `spark/components/` (see [01-2d-rendering](01-2d-rendering.md)).

Next: [2D rendering components](01-2d-rendering.md).
