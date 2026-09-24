# Engine Capabilities

## Rendering Pipeline (High Level)

```mermaid
flowchart LR
    ECS[GameWorld ECS] --> Fill[FillStandardLitSceneFromWorld]
    Fill --> SRP[SceneRenderParams]
    GUI[PaintUiCanvases] --> SRP
    SRP --> VK[VulkanRenderer]
    IMGUI[Dear ImGui overlay] --> VK
    VK --> Present[Swapchain Present]
```

## UI toolkits

| Stack | When to use | Key APIs |
|-------|-------------|----------|
| **Spark UI** (retained, `spark/ui/`) | Menus, editor chrome, themed HUD | `UiCanvasComponent`, `ProcessUiCanvasesInput`, `PaintUiCanvases` |
| **Dear ImGui** (optional, `SPARK_ENABLE_IMGUI`) | Docking tools, debug panels | `IImGuiLayer`, `Ui::UiToolkitSettings`, `DearImguiControlsFactory` or raw ImGui in `OnRender` |

Full guide: [UI and Toolkits](08-ui-and-toolkits.md).

## 3D Rendering

| Feature | Types / Components |
|---------|-------------------|
| Forward PBR | `MaterialComponent`, `SceneShadingModel::LitPbr` |
| Toon / cel | `SceneShadingModel::ToonCel` |
| Directional + CSM | Sun vector in `SceneRenderParams` |
| Point / spot lights | `PointLightComponent`, `SpotLightComponent` (clustered) |
| Skinned characters | `SkinnedMeshComponent`, `AnimatorComponent`, `AttachmentSocketComponent` |
| Billboards / decals / volumes | `BillboardComponent`, `DecalProjectorComponent`, `FogVolumeComponent`, `PostProcessVolumeComponent` |
| 3D camera rigs | `CameraComponent`, `SpringArm3DComponent`, `CameraFollow3DComponent` |
| Time of day | `TimeOfDayDriverComponent` → `SceneRenderParams::timeOfDay` |
| Terrain | `TerrainComponent` (heightfield) |
| Water | `WaterBodyComponent` (Gerstner surface, dedicated pass) |
| Sky | `SkyComponent` + `SceneSkyMode` |
| World clear color | `worldClearColorEnabled` + `worldClearColor` (solid HDR background) |
| Particles | `ParticleEmitterComponent` |
| SSAO / IBL | Fields on `SceneRenderParams` |

## 2D Rendering

| Feature | Types |
|---------|-------|
| Sprites | `SpriteComponent` → `SceneSpriteDraw` |
| Sprite animation | `SpriteAnimatorComponent`, `Sprite2DCharacterAnimFsmComponent`, `SpriteAnimationEventReceiverComponent`, `AnimationHitbox2DComponent` |
| Tilemaps | `TilemapComponent`, `TilemapGameplayGridComponent`, `TilemapMapSourceComponent`, layers/autotile/animator/object helpers |
| TMX import | `TmxImporter`, `ApplyTilemapDocument`, `ResolveTilemapAssetPath` |
| Orthographic camera | `Camera2DComponent`, `Camera2DRigComponent`, `ScreenShakeComponent` |
| Parallax backgrounds | `ParallaxLayerComponent` (camera-relative scroll + optional drift) |
| Y-sort occlusion | `SceneSpriteSortMode::SortOrderThenWorldY` |
| 2D sprite lighting modes | `SpriteLighting2DMode` on draw items |

## 2D Gameplay Components

| Feature | Types |
|---------|-------|
| Platformer motor | `CharacterController2DComponent`, `OneWayPlatform2DComponent` |
| Triggers / pickups | `TriggerVolume2DComponent`, `PickupComponent`, `InteractableComponent` |
| Semantic input | `InputActionMapComponent`, `PlayerInputComponent` |
| Game flow | `GameStateComponent`, `GameFlowTriggerComponent` |
| Grid navigation | `GridNavAgent2DComponent`, `GridPathFollower2DComponent`, `GridNavTarget2DComponent` |
| Combat vitals | `HealthComponent`, `DamageableComponent` |

## Simulation & Tools

```cpp
#include "spark/physics/PhysicsSubsystem.hpp"

PhysicsSubsystem physics;
physics.Simulate2D(world, timing);
physics.SimulateAll3D(world, timing);  // rigidbodies + character + triggers
Spark::SimulateGameAi(world, timing, context);
Spark::ProcessSoundCues(world, context);  // listeners + ambient zones + cue flush
Spark::ProcessUiCanvasesInput(scene, input, fbW, fbH);
```

Legacy free functions (`SimulatePhysics2D`, `SimulatePhysics3D`, …) remain but are **deprecated** — prefer `PhysicsSubsystem`.

**100 built-in components** — full reference: [Game Component Reference](07-game-component-reference.md).

## Asset Loading on GameWorld

```cpp
Spark::GltfAsset asset = world.LoadGltf("assets/models/Cube.glb");
Spark::SkinnedGltfAsset fox = world.LoadSkinnedGltf("assets/models/Fox.glb");
auto tex = world.LoadTexture("assets/sprites/player.png");
world.RegisterMesh(mesh, "my_game/hero");
world.RegisterTexture(tex, "my_game/hero_albedo");
```

Next: [Building and Running](03-building-and-running.md).
