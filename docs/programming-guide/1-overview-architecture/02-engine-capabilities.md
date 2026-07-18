---
title: Engine Capabilities
order: 2
---

# Engine Capabilities

## Rendering Pipeline (High Level)

```mermaid
flowchart LR
    ECS[GameWorld ECS] --> Fill[FillStandardLitSceneFromWorld]
    Fill --> SRP[SceneRenderParams]
    GUI[PaintGuiCanvases] --> SRP
    SRP --> VK[VulkanRenderer]
    VK --> Present[Swapchain Present]
```

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
| Sky | `SkyComponent` + `SceneSkyMode` |
| Particles | `ParticleEmitterComponent` |
| SSAO / IBL | Fields on `SceneRenderParams` |

## 2D Rendering

| Feature | Types |
|---------|-------|
| Sprites | `SpriteComponent` → `SceneSpriteDraw` |
| Tilemaps | `TilemapComponent` |
| Orthographic camera | `Camera2DComponent`, `Camera2DRigComponent` |
| Y-sort occlusion | `SceneSpriteSortMode::SortOrderThenWorldY` |
| 2D sprite lighting modes | `SpriteLighting2DMode` on draw items |

## Simulation & Tools

```cpp
Spark::SimulatePhysics2D(world, timing, settings);
Spark::SimulatePhysics3D(world, timing, settings);
Spark::SimulateCharacterControllers3D(world, timing);
Spark::SimulateTriggerVolumes3D(world, timing);
Spark::SimulateGameAi(world, timing, context);
Spark::ProcessSoundCues(world, context);  // listeners + ambient zones + cue flush
Spark::ProcessGuiCanvasesInput(scene, input, fbW, fbH);
```

**64 built-in components** — full reference: [Game Component Reference](07-game-component-reference.html).

## Asset Loading on GameWorld

```cpp
Spark::GltfAsset asset = world.LoadGltf("assets/models/Cube.glb");
Spark::SkinnedGltfAsset fox = world.LoadSkinnedGltf("assets/models/Fox.glb");
auto tex = world.LoadTexture("assets/sprites/player.png");
world.RegisterMesh(mesh, "my_game/hero");
world.RegisterTexture(tex, "my_game/hero_albedo");
```

Next: [Building and Running](overview-architecture/03-building-and-running.html).
