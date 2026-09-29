# Part 7 — 2D game (overview)

**Part 7** is the single home for **2D gameplay**: side-view platformers, top-down tilemap games, grid navigation, combat, HUD, audio, and the P0 product path. **Part 2** (`2-2d-graphics/`) covers rendering primitives (sprites, tile drawing, cameras as math).

## SparkDemo references

| Demo | Key | Focus |
|------|-----|-------|
| `Platformer2DDemo` | **#6** | `CharacterController2D`, combat hitboxes, parallax, game flow |
| `TilemapShowcase2DDemo` | **#19** | TMX, gameplay grid, click-to-move |
| `GameFlow2DProductPathDemo` | **#26 / F** | `.sparkscene`, pools, patrol/chaser nav, GPU minimap, sprite FX |

## Chapters in this part

| Ch. | Guide | Topics |
|-----|--------|--------|
| [01](01-platformer-intro.md) | Platformer intro | Demo layout, component stack |
| [02](02-project-setup.md) | Project setup | CMake, frame order, fonts, textures |
| [03](03-level-design.md) | Level design | Tilemaps, collision, object layers |
| [04](04-player-controller.md) | Player controller | Input, motor, combat, grid nav |
| [05](05-camera-hud.md) | Camera & HUD | Rig, shake, parallax, minimap, UI |
| [06](06-polish.md) | Polish | Juice, animation events, sound, FX |
| [07](07-p0-2d-product-path.md) | P0 product path | Authored scenes, gameplay table, saves |
| [08](08-scene2d-runtime-limits.md) | Runtime limits | `SceneRenderParams` caps, composite RT |
| [09](09-2d-gameplay-api-guide.md) | Gameplay API | End-to-end tilemap + nav + pools |
| [10](10-2d-components-handbook.md) | **Components handbook** | **Every built-in component** (2D usage + 3D pointers) |

For field-level API detail, keep [Game component reference](../1-overview-architecture/07-game-component-reference.md) open; [chapter 10](10-2d-components-handbook.md) lists every `ComponentKind` one-to-one.

## Typical stacks

**Side-view platformer:** `CharacterController2D` + `Sprite2DCharacterAnimFsm` + `AnimationHitbox2D` + `GameState` / `GameFlowTrigger`.

**Top-down grid:** `TilemapGameplayGrid` + `GridNavAgent2D` + `Rigidbody2D` + `Scene2DCompositeView` (minimap).

Next: [01 — Platformer introduction](01-platformer-intro.md).
