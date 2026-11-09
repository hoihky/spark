# Part 9 — 3D game (overview)

**Part 9** collects **3D gameplay walkthroughs** and ties them to SparkDemo. Rendering primitives live in [Part 3](../3-3d-graphics/01-meshes-and-materials.md); attach recipes in [Part 7](../7-game-components-usage/02-3d-rendering.md).

## SparkDemo references

| Demo | Key | Focus |
|------|-----|-------|
| `ThreeDDemo` / Basic 3D | **#1** | glTF, PBR, skinned fox |
| `CharacterCameraDemo` | **#5** | 1st/3rd person, CC3D, spring arm |
| `Maze3DDemo` | **#8** | Nav mesh guard, perception, gems |
| `PhysicsShowcase3DDemo` | **#29 / P** | Layers, triggers, carry, CCD, replay |
| `TimeOfDayDemo` | **#17 / N** | TOD, fog/post volumes — [ch.10](../3-3d-graphics/10-post-process-fog-volumes.md) |
| `WaterLakeDemo` | **#23 / W** | Island, ocean, SSR — [environment stack](../3-3d-graphics/09-environment-stack.md) |
| `FoliageWindDemo` / `GrassFieldTerrainDemo` | **#27–28** | Foliage + grass on terrain — [environment stack](../3-3d-graphics/09-environment-stack.md) |
| `GameFlow2DProductPathDemo` | — | (2D) see [Part 8](../8-2d-game/00-2d-game-systems-map.md) |

Custom **FPS arena** chapters (**01–06**) combine fly camera + hitscan from `SceneEditor3DDemo` / throw demo patterns.

## Chapters in this part

| Ch. | Guide | Topics |
|-----|--------|--------|
| [01](01-fps-intro.md) | FPS intro | Minimal arena scope |
| [02](02-project-setup.md) | Project setup | Meshes, physics hook |
| [03](03-arena.md) | Arena | Ground + targets |
| [04](04-shooting.md) | Shooting | Hitscan, tracers |
| [05](05-rendering.md) | Rendering | `SubmitStandardLitSceneFromWorld` |
| [06](06-extending.md) | Extending | Weapons, glTF, AI hints |
| [07](07-character-and-third-person.md) | Character camera | CC3D, spring arm, skinned glTF |
| [08](08-physics-3d-product-path.md) | Physics 3D path | Showcase **P** integration |
| [09](09-maze-ai-and-perception.md) | Maze AI | Patrol, perception, nav mesh |
| [10](10-3d-components-handbook.md) | **3D handbook** | Component stacks for FPS / character / environment |

Cross-cutting: [Assets](../1-overview-architecture/12-assets-and-content-pipeline.md) · [Scenes](../1-overview-architecture/11-scenes-and-saves.md) · [Demo catalog](../1-overview-architecture/10-sparkdemo-catalog.md).

## Typical stacks

**Fly / arena FPS:** `FlyCamera` + mesh targets + analytic raycast (ch. **01–06**).

**Character action:** `CharacterController3D` + `SpringArm3D` + `CameraFollow3D` + skinned mesh (ch. **07**).

**Physics sandbox:** `Rigidbody3D` + triggers + query layers + `SimulateAll3D` (ch. **08**).

**Stealth / chase:** `NavMeshAgent` + `PerceptionSensor` + `AiAgent` (ch. **09**).

Next: [01 — FPS introduction](01-fps-intro.md).
