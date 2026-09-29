# 2D components handbook

One **`###` section per `ComponentKind`** value in `include/spark/ecs/GameComponent.hpp` (**108** kinds, excluding `Unknown`). Order below matches the enum. Full field-level API: [Game component reference](../1-overview-architecture/07-game-component-reference.md).

**Include:** `#include "spark/ecs/Ecs.hpp"` or the header listed on each row.

**2D role:** **Primary** — common in pure 2D games; **Optional** — useful in 2.5D or mixed scenes; **3D-only** — skip for flat sprite/tilemap games (see [Part 3](../3-3d-graphics/01-meshes-and-materials.md) / [Part 8](../8-3d-game/01-fps-intro.md)).

### Quick jumps (Part 7 chapters)

| Topic | Anchor |
|-------|--------|
| Tilemap stack | [#tilemap](#tilemap) · [#tilemapgameplaygrid](#tilemapgameplaygrid) · [#tilemapobjectspawn](#tilemapobjectspawn) |
| Player / combat | [#charactercontroller2d](#charactercontroller2d) · [#animationhitbox2d](#animationhitbox2d) · [#projectile2d](#projectile2d) |
| Camera / HUD | [#camera2d](#camera2d) · [#parallaxlayer](#parallaxlayer) · [#scene2dcompositeview](#scene2dcompositeview) |
| Grid AI | [#gridnavagent2d](#gridnavagent2d) · [#aiagent](#aiagent) |
| Input / flow | [#playerinput](#playerinput) · [#gamestate](#gamestate) |

---

## Master index (`ComponentKind` order)

| Kind | Component | 2D |
|------|-----------|-----|
| `Transform` | `TransformComponent` | Primary |
| `Mesh` | `MeshComponent` | 3D-only |
| `Collision` | `CollisionComponent` | Optional |
| `Material` | `MaterialComponent` | 3D-only |
| `MultiMaterial` | `MultiMaterialComponent` | 3D-only |
| `PointLight` | `PointLightComponent` | Optional |
| `SkinnedMesh` | `SkinnedMeshComponent` | 3D-only |
| `Animator` | `AnimatorComponent` | 3D-only |
| `TextOverlay` | `TextOverlayComponent` | Primary |
| `UiCanvas` | `UiCanvasComponent` | Primary |
| `Sky` | `SkyComponent` | 3D-only |
| `ParticleEmitter` | `ParticleEmitterComponent` | Primary |
| `VfxPlayer` | `VfxPlayerComponent` | Optional |
| `Terrain` | `TerrainComponent` | 3D-only |
| `Sprite` | `SpriteComponent` | Primary |
| `Tilemap` | `TilemapComponent` | Primary |
| `TilemapCollider2D` | `TilemapCollider2DComponent` | Primary |
| `BoxCollider2D` | `BoxCollider2DComponent` | Primary |
| `Rigidbody2D` | `Rigidbody2DComponent` | Primary |
| `CharacterController2D` | `CharacterController2DComponent` | Primary |
| `OneWayPlatform2D` | `OneWayPlatform2DComponent` | Primary |
| `TriggerVolume2D` | `TriggerVolume2DComponent` | Primary |
| `SpriteAnimator` | `SpriteAnimatorComponent` | Primary |
| `CircleCollider2D` | `CircleCollider2DComponent` | Primary |
| `SpriteLighting2D` | `SpriteLighting2DComponent` | Primary |
| `BlendMode` | `BlendModeComponent` | Primary |
| `BoxCollider3D` | `BoxCollider3DComponent` | 3D-only |
| `SphereCollider3D` | `SphereCollider3DComponent` | 3D-only |
| `CapsuleCollider3D` | `CapsuleCollider3DComponent` | 3D-only |
| `Rigidbody3D` | `Rigidbody3DComponent` | 3D-only |
| `PhysicsMaterial3D` | `PhysicsMaterial3DComponent` | 3D-only |
| `DistanceJoint3D` | `DistanceJoint3DComponent` | 3D-only |
| `SceneSpatialPolicy` | `SceneSpatialPolicyComponent` | Optional |
| `AiAgent` | `AiAgentComponent` | Primary |
| `SoundCue` | `SoundCueComponent` | Primary |
| `Sprite2DCharacterAnimFsm` | `Sprite2DCharacterAnimFsmComponent` | Primary |
| `Character3DAnimFsm` | `Character3DAnimFsmComponent` | 3D-only |
| `SpotLight` | `SpotLightComponent` | Optional |
| `DirectionalLight` | `DirectionalLightComponent` | Optional |
| `Camera` | `CameraComponent` | 3D-only |
| `Camera2D` | `Camera2DComponent` | Primary |
| `Camera2DRig` | `Camera2DRigComponent` | Primary |
| `RenderLayer` | `RenderLayerComponent` | Primary |
| `SortingGroup` | `SortingGroupComponent` | Primary |
| `CharacterController3D` | `CharacterController3DComponent` | 3D-only |
| `TriggerVolume3D` | `TriggerVolume3DComponent` | 3D-only |
| `AudioListener` | `AudioListenerComponent` | Primary |
| `Billboard` | `BillboardComponent` | Optional |
| `AnimationEventReceiver` | `AnimationEventReceiverComponent` | 3D-only |
| `AnimationEventVfx` | `AnimationEventVfxComponent` | 3D-only |
| `AnimationMeleeHit` | `AnimationMeleeHitComponent` | 3D-only |
| `RootMotion` | `RootMotionComponent` | 3D-only |
| `AttachmentSocket` | `AttachmentSocketComponent` | 3D-only |
| `SkeletonDebugDraw` | `SkeletonDebugDrawComponent` | 3D-only |
| `FootIk` | `FootIkComponent` | 3D-only |
| `AimIk` | `AimIkComponent` | 3D-only |
| `SkinnedAnimationBudget` | `SkinnedAnimationBudgetComponent` | 3D-only |
| `CameraFollow3D` | `CameraFollow3DComponent` | 3D-only |
| `SpringArm3D` | `SpringArm3DComponent` | 3D-only |
| `PolygonCollider2D` | `PolygonCollider2DComponent` | Primary |
| `Health` | `HealthComponent` | Primary |
| `Damageable` | `DamageableComponent` | Primary |
| `Interactable` | `InteractableComponent` | Primary |
| `Pickup` | `PickupComponent` | Primary |
| `AnimationHitbox2D` | `AnimationHitbox2DComponent` | Primary |
| `Hurtbox2D` | `Hurtbox2DComponent` | Primary |
| `Projectile2D` | `Projectile2DComponent` | Primary |
| `DamageZone2D` | `DamageZone2DComponent` | Primary |
| `SpawnPoint2D` | `SpawnPoint2DComponent` | Primary |
| `CameraBounds2D` | `CameraBounds2DComponent` | Primary |
| `ParallaxLayer` | `ParallaxLayerComponent` | Primary |
| `ScreenShake` | `ScreenShakeComponent` | Primary |
| `SpriteAnimationEventReceiver` | `SpriteAnimationEventReceiverComponent` | Primary |
| `SpriteAnimationEventVfx` | `SpriteAnimationEventVfxComponent` | Primary |
| `InputActionMap` | `InputActionMapComponent` | Primary |
| `PlayerInput` | `PlayerInputComponent` | Primary |
| `GameState` | `GameStateComponent` | Primary |
| `GameFlowTrigger` | `GameFlowTriggerComponent` | Primary |
| `DecalProjector` | `DecalProjectorComponent` | 3D-only |
| `PhysicsMaterial2D` | `PhysicsMaterial2DComponent` | Primary |
| `MeshCollider3D` | `MeshCollider3DComponent` | 3D-only |
| `NavMeshAgent` | `NavMeshAgentComponent` | 3D-only |
| `GridNavAgent2D` | `GridNavAgent2DComponent` | Primary |
| `GridPathFollower2D` | `GridPathFollower2DComponent` | Primary |
| `GridNavTarget2D` | `GridNavTarget2DComponent` | Primary |
| `PatrolPath` | `PatrolPathComponent` | Optional |
| `PerceptionSensor` | `PerceptionSensorComponent` | Primary |
| `AmbientZone` | `AmbientZoneComponent` | Primary |
| `FogVolume` | `FogVolumeComponent` | 3D-only |
| `PostProcessVolume` | `PostProcessVolumeComponent` | 3D-only |
| `HingeJoint3D` | `HingeJoint3DComponent` | 3D-only |
| `SpringJoint3D` | `SpringJoint3DComponent` | 3D-only |
| `DistanceJoint2D` | `DistanceJoint2DComponent` | Primary |
| `HingeJoint2D` | `HingeJoint2DComponent` | Primary |
| `TilemapGameplayGrid` | `TilemapGameplayGridComponent` | Primary |
| `TilemapTileAnimator` | `TilemapTileAnimatorComponent` | Primary |
| `TilemapAutotile` | `TilemapAutotileComponent` | Primary |
| `TilemapObjectLayer` | `TilemapObjectLayerComponent` | Primary |
| `TilemapObjectSpawn` | `TilemapObjectSpawnComponent` | Primary |
| `TilemapObjectGizmo` | `TilemapObjectGizmoComponent` | Primary |
| `TilemapMapSource` | `TilemapMapSourceComponent` | Primary |
| `TimeOfDayDriver` | `TimeOfDayDriverComponent` | Optional |
| `SpawnPoint` | `SpawnPointComponent` | 3D-only |
| `GltfSceneSource` | `GltfSceneSourceComponent` | 3D-only |
| `GltfInstanceNode` | `GltfInstanceNodeComponent` | 3D-only |
| `WaterBody` | `WaterBodyComponent` | 3D-only |
| `PointLight2D` | `PointLight2DComponent` | Primary |
| `Scene2DCompositeView` | `Scene2DCompositeViewComponent` | Primary |

---

## Catalog (enum order)

### `TransformComponent` {#transform}

| **Kind** | `ComponentKind::Transform` |
| **Header** | `spark/ecs/components/core/TransformComponent.hpp` |
| **2D** | Primary — every drawable / physics object |

Local TRS; Z translation and sprite `sortOrder` control draw order.

```cpp
auto* tr = go->AddComponent<TransformComponent>();
tr->SetTranslation({x, y, 0.05F});
tr->SetRotation(Quaternion::FromAxisAngle(Vector3::UnitZ, angleRadians));
```

### `MeshComponent` {#mesh}

| **Kind** | `ComponentKind::Mesh` |
| **Header** | `spark/ecs/components/rendering/MeshComponent.hpp` |
| **2D** | 3D-only |

Static mesh draw in the lit 3D scene. Not used for sprite/tilemap games.

### `CollisionComponent` {#collision}

| **Kind** | `ComponentKind::Collision` |
| **Header** | `spark/ecs/components/physics/CollisionComponent.hpp` |
| **2D** | Optional — legacy world-space sphere bounds |

Simple sphere probe; prefer `BoxCollider2D` / `CircleCollider2D` for gameplay.

```cpp
go->AddComponent<CollisionComponent>(0.5F, Vector3::Zero);
```

### `MaterialComponent` {#material}

| **Kind** | `ComponentKind::Material` |
| **Header** | `spark/ecs/components/rendering/MaterialComponent.hpp` |
| **2D** | 3D-only |

PBR/toon material on `MeshComponent` / `SkinnedMeshComponent`.

### `MultiMaterialComponent` {#multimaterial}

| **Kind** | `ComponentKind::MultiMaterial` |
| **Header** | `spark/ecs/components/rendering/MultiMaterialComponent.hpp` |
| **2D** | 3D-only |

Per-submesh material slots for multi-material glTF meshes (`PopulateFromGltfAsset`).

### `PointLightComponent` {#pointlight}

| **Kind** | `ComponentKind::PointLight` |
| **Header** | `spark/ecs/components/lighting/PointLightComponent.hpp` |
| **2D** | Optional — lights 3D meshes; sprites use sun + `PointLight2D` |

```cpp
go->AddComponent<PointLightComponent>(Vector3{1,0.9,0.8}, 4.0F, 12.0F);
```

### `SkinnedMeshComponent` {#skinnedmesh}

| **Kind** | `ComponentKind::SkinnedMesh` |
| **Header** | `spark/ecs/components/rendering/SkinnedMeshComponent.hpp` |
| **2D** | 3D-only |

Skeletal mesh + joint palette. Pair with `AnimatorComponent`.

### `AnimatorComponent` {#animator}

| **Kind** | `ComponentKind::Animator` |
| **Header** | `spark/ecs/components/animation/AnimatorComponent.hpp` |
| **2D** | 3D-only — use `SpriteAnimatorComponent` for flipbooks |

Clip playback on skeleton assets (priority 200).

### `TextOverlayComponent` {#textoverlay}

| **Kind** | `ComponentKind::TextOverlay` |
| **Header** | `spark/ecs/components/rendering/TextOverlayComponent.hpp` |
| **2D** | Primary — screen HUD |

Requires `GameWorld::SetUiFont`.

```cpp
world.SetUiFont(font);
auto* hud = hudGo->AddComponent<TextOverlayComponent>();
hud->SetText("Gems: 3/5");
hud->SetScreenPosition({20.0F, 20.0F});
```

### `UiCanvasComponent` {#uicanvas}

| **Kind** | `ComponentKind::UiCanvas` |
| **Header** | `spark/ecs/components/ui/UiCanvasComponent.hpp` |
| **2D** | Primary — retained UI |

See [UI and toolkits](../1-overview-architecture/08-ui-and-toolkits.md).

```cpp
auto* canvas = uiRoot->AddComponent<UiCanvasComponent>();
canvas->SetSortOrder(0);
```

### `SkyComponent` {#sky}

| **Kind** | `ComponentKind::Sky` |
| **Header** | `spark/ecs/components/rendering/SkyComponent.hpp` |
| **2D** | 3D-only |

Skybox mode on a sky mesh. 2D games use parallax sprites instead.

### `ParticleEmitterComponent` {#particleemitter}

| **Kind** | `ComponentKind::ParticleEmitter` |
| **Header** | `spark/ecs/components/rendering/ParticleEmitterComponent.hpp` |
| **2D** | Primary |

```cpp
auto* burst = go->AddComponent<ParticleEmitterComponent>();
burst->SetRenderSpace(ParticleRenderSpace::SpriteLayer);
burst->SetEffectId("hit_spark_2d");
burst->SetEmissionRate(40.0F);
```

### `VfxPlayerComponent` {#vfxplayer}

| **Kind** | `ComponentKind::VfxPlayer` |
| **Header** | `spark/ecs/components/rendering/VfxPlayerComponent.hpp` |
| **2D** | Optional — plays `VfxAsset` (particles / prefab) |

```cpp
auto* vfx = go->AddComponent<VfxPlayerComponent>();
vfx->SetVfxAssetKey("collect_burst");
vfx->SetPlayOnStartOnce(true);
```

### `TerrainComponent` {#terrain}

| **Kind** | `ComponentKind::Terrain` |
| **Header** | `spark/ecs/components/rendering/TerrainComponent.hpp` |
| **2D** | 3D-only |

Heightfield terrain mesh. Use `TilemapComponent` for 2D levels.

### `SpriteComponent` {#sprite}

| **Kind** | `ComponentKind::Sprite` |
| **Header** | `spark/ecs/components/rendering/SpriteComponent.hpp` |
| **2D** | Primary |

```cpp
go->AddComponent<SpriteComponent>(texture, Vector4{1,1,1,1}, uvRect, sortOrder);
```

### `TilemapComponent` {#tilemap}

| **Kind** | `ComponentKind::Tilemap` |
| **Header** | `spark/ecs/components/rendering/TilemapComponent.hpp` |
| **2D** | Primary |

Layer grid + tileset. See [Tilemaps](../2-2d-graphics/03-tilemaps.md), [03 — Level design](03-level-design.md).

```cpp
auto* tm = go->AddComponent<TilemapComponent>(tileset, width, height, cellSize, 0);
tm->AddLayer("Ground");
tm->BakeGameplayGrid(grid, TilemapGameplayWalkRule::DefinitionAndFlags);
```

### `TilemapCollider2DComponent` {#tilemapcollider2d}

| **Kind** | `ComponentKind::TilemapCollider2D` |
| **Header** | `spark/ecs/components/physics/2d/TilemapCollider2DComponent.hpp` |
| **2D** | Primary — static collision from painted tiles |

Sibling on the same object as `TilemapComponent`.

```cpp
go->AddComponent<TilemapCollider2DComponent>();
```

### `BoxCollider2DComponent` {#boxcollider2d}

| **Kind** | `ComponentKind::BoxCollider2D` |
| **Header** | `spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp` |
| **2D** | Primary |

```cpp
auto* box = go->AddComponent<BoxCollider2DComponent>(Vector2{0.5F, 0.5F});
box->SetCategoryBits(1u << 0);
box->SetMaskBits(0xFFFF);
```

### `Rigidbody2DComponent` {#rigidbody2d}

| **Kind** | `ComponentKind::Rigidbody2D` |
| **Header** | `spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp` |
| **2D** | Primary |

Step with `PhysicsSubsystem::Simulate2D`. Grid-nav NPCs often use dynamic bodies.

```cpp
auto* rb = go->AddComponent<Rigidbody2DComponent>(RigidbodyBodyType2D::Dynamic, 1.0F);
rb->SetVelocity({vx, vy});
```

### `CharacterController2DComponent` {#charactercontroller2d}

| **Kind** | `ComponentKind::CharacterController2D` |
| **Header** | `spark/ecs/components/physics/2d/CharacterController2DComponent.hpp` |
| **2D** | Primary — platformer motor |

Set move/jump **before** `Simulate2D`. See [04 — Player controller](04-player-controller.md).

```cpp
auto* cc = player->AddComponent<CharacterController2DComponent>();
cc->SetMoveInputX(input->GetActionAxis1D("MoveX"));
if (input->WasActionPressedThisFrame("Jump")) {
    cc->RequestJump();
}
```

### `OneWayPlatform2DComponent` {#onewayplatform2d}

| **Kind** | `ComponentKind::OneWayPlatform2D` |
| **Header** | `spark/ecs/components/physics/2d/OneWayPlatform2DComponent.hpp` |
| **2D** | Primary |

Marker on static platform colliders; works with `CharacterController2D`.

```cpp
platform->AddComponent<OneWayPlatform2DComponent>();
```

### `TriggerVolume2DComponent` {#triggervolume2d}

| **Kind** | `ComponentKind::TriggerVolume2D` |
| **Header** | `spark/ecs/components/physics/2d/TriggerVolume2DComponent.hpp` |
| **2D** | Primary — goals, pickups, zones |

Emits `Physics2DTriggerEnter` / `Stay` / `Exit` to sibling components.

```cpp
gem->AddComponent<TriggerVolume2DComponent>(TriggerVolume2DShape::Circle);
gem->GetComponent<TriggerVolume2DComponent>()->SetRadius(0.45F);
```

### `SpriteAnimatorComponent` {#spriteanimator}

| **Kind** | `ComponentKind::SpriteAnimator` |
| **Header** | `spark/ecs/components/animation/SpriteAnimatorComponent.hpp` |
| **2D** | Primary — flipbook playback (priority 200) |

```cpp
auto* sa = go->AddComponent<SpriteAnimatorComponent>();
sa->SetUniformGrid(4, 4);
sa->SetClipIndex(0);
```

### `CircleCollider2DComponent` {#circlecollider2d}

| **Kind** | `ComponentKind::CircleCollider2D` |
| **Header** | `spark/ecs/components/physics/2d/CircleCollider2DComponent.hpp` |
| **2D** | Primary |

On a dynamic body, circle shape wins over box if both exist.

```cpp
go->AddComponent<CircleCollider2DComponent>(0.45F);
```

### `SpriteLighting2DComponent` {#spritelighting2d}

| **Kind** | `ComponentKind::SpriteLighting2D` |
| **Header** | `spark/ecs/components/rendering/SpriteLighting2DComponent.hpp` |
| **2D** | Primary — outline, hit flash, dissolve |

Same object as `SpriteComponent`. See [2D lighting](../2-2d-graphics/05-2d-lighting.md), [06 — Polish](06-polish.md).

```cpp
go->AddComponent<SpriteLighting2DComponent>();
// ApplyDissolveProgress, ApplyHitFlash, etc. from gameplay code
```

### `BlendModeComponent` {#blendmode}

| **Kind** | `ComponentKind::BlendMode` |
| **Header** | `spark/ecs/components/rendering/BlendModeComponent.hpp` |
| **2D** | Primary |

```cpp
fx->AddComponent<BlendModeComponent>(SceneBlendMode::Additive);
```

### `BoxCollider3DComponent` {#boxcollider3d}

| **Kind** | `ComponentKind::BoxCollider3D` |
| **Header** | `spark/ecs/components/physics/3d/BoxCollider3DComponent.hpp` |
| **2D** | 3D-only |

### `SphereCollider3DComponent` {#spherecollider3d}

| **Kind** | `ComponentKind::SphereCollider3D` |
| **Header** | `spark/ecs/components/physics/3d/SphereCollider3DComponent.hpp` |
| **2D** | 3D-only |

### `CapsuleCollider3DComponent` {#capsulecollider3d}

| **Kind** | `ComponentKind::CapsuleCollider3D` |
| **Header** | `spark/ecs/components/physics/3d/CapsuleCollider3DComponent.hpp` |
| **2D** | 3D-only |

### `Rigidbody3DComponent` {#rigidbody3d}

| **Kind** | `ComponentKind::Rigidbody3D` |
| **Header** | `spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp` |
| **2D** | 3D-only |

### `PhysicsMaterial3DComponent` {#physicsmaterial3d}

| **Kind** | `ComponentKind::PhysicsMaterial3D` |
| **Header** | `spark/ecs/components/physics/3d/PhysicsMaterial3DComponent.hpp` |
| **2D** | 3D-only |

### `DistanceJoint3DComponent` {#distancejoint3d}

| **Kind** | `ComponentKind::DistanceJoint3D` |
| **Header** | `spark/ecs/components/physics/3d/DistanceJoint3DComponent.hpp` |
| **2D** | 3D-only |

### `SceneSpatialPolicyComponent` {#scenespatialpolicy}

| **Kind** | `ComponentKind::SceneSpatialPolicy` |
| **Header** | `spark/ecs/components/world/SceneSpatialPolicyComponent.hpp` |
| **2D** | Optional — scene partition for large worlds |

```cpp
go->AddComponent<SceneSpatialPolicyComponent>(ScenePartitionKind::BoundingVolumeHierarchy);
```

### `AiAgentComponent` {#aiagent}

| **Kind** | `ComponentKind::AiAgent` |
| **Header** | `spark/ecs/components/ai/AiAgentComponent.hpp` |
| **2D** | Primary — steering on XY (`AiSteeringPlane::XyRigidbody2D`) |

Ticked in `SimulateGameAi`, not `OnUpdate`.

```cpp
auto* agent = enemy->AddComponent<AiAgentComponent>();
agent->SetSteeringPlane(AiSteeringPlane::XyRigidbody2D);
agent->SetMaxSpeed(4.0F);
```

### `SoundCueComponent` {#soundcue}

| **Kind** | `ComponentKind::SoundCue` |
| **Header** | `spark/ecs/components/audio/SoundCueComponent.hpp` |
| **2D** | Primary |

```cpp
go->AddComponent<SoundCueComponent>()->Queue(clip, 0.8F);
```

### `Sprite2DCharacterAnimFsmComponent` {#sprite2dcharacteranimfsm}

| **Kind** | `ComponentKind::Sprite2DCharacterAnimFsm` |
| **Header** | `spark/ecs/components/animation/Sprite2DCharacterAnimFsmComponent.hpp` |
| **2D** | Primary — idle/run/attack FSM (priority 100) |

```cpp
go->AddComponent<Sprite2DCharacterAnimFsmComponent>();
go->GetComponent<Sprite2DCharacterAnimFsmComponent>()->RequestAttack();
```

### `Character3DAnimFsmComponent` {#character3danimfsm}

| **Kind** | `ComponentKind::Character3DAnimFsm` |
| **Header** | `spark/ecs/components/animation/Character3DAnimFsmComponent.hpp` |
| **2D** | 3D-only |

Locomotion/combat driver for `AnimatorComponent`.

### `SpotLightComponent` {#spotlight}

| **Kind** | `ComponentKind::SpotLight` |
| **Header** | `spark/ecs/components/lighting/SpotLightComponent.hpp` |
| **2D** | Optional |

```cpp
go->AddComponent<SpotLightComponent>(Vector3::One, 6.0F, 15.0F, 25.0F, 40.0F);
```

### `DirectionalLightComponent` {#directionallight}

| **Kind** | `ComponentKind::DirectionalLight` |
| **Header** | `spark/ecs/components/lighting/DirectionalLightComponent.hpp` |
| **2D** | Optional — sun vector for lit sprites |

Direction = owner transform local **+Z** in world space.

```cpp
sunGo->AddComponent<DirectionalLightComponent>(Vector3{1,0.97,0.9}, 1.2F);
```

### `CameraComponent` {#camera}

| **Kind** | `ComponentKind::Camera` |
| **Header** | `spark/ecs/components/camera/CameraComponent.hpp` |
| **2D** | 3D-only — use `Camera2DComponent` |

Perspective / orthographic 3D main camera selection by `priority`.

### `Camera2DComponent` {#camera2d}

| **Kind** | `ComponentKind::Camera2D` |
| **Header** | `spark/ecs/components/camera/Camera2DComponent.hpp` |
| **2D** | Primary |

```cpp
cameraGo->AddComponent<Camera2DComponent>()->SetHalfExtentY(6.0F);
```

### `Camera2DRigComponent` {#camera2drig}

| **Kind** | `ComponentKind::Camera2DRig` |
| **Header** | `spark/ecs/components/camera/Camera2DRigComponent.hpp` |
| **2D** | Primary — follow, look-ahead, shake offset |

```cpp
auto* rig = cameraGo->AddComponent<Camera2DRigComponent>();
rig->SetTarget(player);
rig->SetFollowSmoothRate(7.5F);
```

See [05 — Camera and HUD](05-camera-hud.md).

### `RenderLayerComponent` {#renderlayer}

| **Kind** | `ComponentKind::RenderLayer` |
| **Header** | `spark/ecs/components/rendering/RenderLayerComponent.hpp` |
| **2D** | Primary |

```cpp
hero->AddComponent<RenderLayerComponent>("Characters", 10);
```

### `SortingGroupComponent` {#sortinggroup}

| **Kind** | `ComponentKind::SortingGroup` |
| **Header** | `spark/ecs/components/rendering/SortingGroupComponent.hpp` |
| **2D** | Primary |

```cpp
propsRoot->AddComponent<SortingGroupComponent>(50);
```

### `CharacterController3DComponent` {#charactercontroller3d}

| **Kind** | `ComponentKind::CharacterController3D` |
| **Header** | `spark/ecs/components/physics/3d/CharacterController3DComponent.hpp` |
| **2D** | 3D-only |

### `TriggerVolume3DComponent` {#triggervolume3d}

| **Kind** | `ComponentKind::TriggerVolume3D` |
| **Header** | `spark/ecs/components/physics/3d/TriggerVolume3DComponent.hpp` |
| **2D** | 3D-only |

### `AudioListenerComponent` {#audiolistener}

| **Kind** | `ComponentKind::AudioListener` |
| **Header** | `spark/ecs/components/audio/AudioListenerComponent.hpp` |
| **2D** | Primary — attach to camera rig |

```cpp
cameraGo->AddComponent<AudioListenerComponent>()->SetPriority(10);
```

### `BillboardComponent` {#billboard}

| **Kind** | `ComponentKind::Billboard` |
| **Header** | `spark/ecs/components/rendering/BillboardComponent.hpp` |
| **2D** | Optional — 2.5D props face camera |

```cpp
sign->AddComponent<SpriteComponent>(iconTex);
sign->AddComponent<BillboardComponent>();
```

### `AnimationEventReceiverComponent` {#animationeventreceiver}

| **Kind** | `ComponentKind::AnimationEventReceiver` |
| **Header** | `spark/ecs/components/animation/AnimationEventReceiverComponent.hpp` |
| **2D** | 3D-only — skeletal clip markers (`SignalId::AnimationEvent`) |

### `AnimationEventVfxComponent` {#animationeventvfx}

| **Kind** | `ComponentKind::AnimationEventVfx` |
| **Header** | `spark/ecs/components/animation/AnimationEventVfxComponent.hpp` |
| **2D** | 3D-only — maps animation events to VFX keys |

### `AnimationMeleeHitComponent` {#animationmeleehit}

| **Kind** | `ComponentKind::AnimationMeleeHit` |
| **Header** | `spark/ecs/components/animation/AnimationMeleeHitComponent.hpp` |
| **2D** | 3D-only — `active_start` / `active_end` swing traces |

```cpp
auto* melee = character->AddComponent<AnimationMeleeHitComponent>();
melee->SetStartEventName("active_start");
melee->SetEndEventName("active_end");
```

### `RootMotionComponent` {#rootmotion}

| **Kind** | `ComponentKind::RootMotion` |
| **Header** | `spark/ecs/components/animation/RootMotionComponent.hpp` |
| **2D** | 3D-only — applies skeletal root motion to transform |

### `AttachmentSocketComponent` {#attachmentsocket}

| **Kind** | `ComponentKind::AttachmentSocket` |
| **Header** | `spark/ecs/components/animation/AttachmentSocketComponent.hpp` |
| **2D** | 3D-only — weapon bone parenting |

### `SkeletonDebugDrawComponent` {#skeletondebugdraw}

| **Kind** | `ComponentKind::SkeletonDebugDraw` |
| **Header** | `spark/ecs/components/animation/SkeletonDebugDrawComponent.hpp` |
| **2D** | 3D-only — debug skeleton lines |

### `FootIkComponent` {#footik}

| **Kind** | `ComponentKind::FootIk` |
| **Header** | `spark/ecs/components/animation/FootIkComponent.hpp` |
| **2D** | 3D-only |

### `AimIkComponent` {#aimik}

| **Kind** | `ComponentKind::AimIk` |
| **Header** | `spark/ecs/components/animation/AimIkComponent.hpp` |
| **2D** | 3D-only |

### `SkinnedAnimationBudgetComponent` {#skinnedanimationbudget}

| **Kind** | `ComponentKind::SkinnedAnimationBudget` |
| **Header** | `spark/ecs/components/animation/SkinnedAnimationBudgetComponent.hpp` |
| **2D** | 3D-only — LOD / budget for skinned characters |

### `CameraFollow3DComponent` {#camerafollow3d}

| **Kind** | `ComponentKind::CameraFollow3D` |
| **Header** | `spark/ecs/components/camera/CameraFollow3DComponent.hpp` |
| **2D** | 3D-only |

### `SpringArm3DComponent` {#springarm3d}

| **Kind** | `ComponentKind::SpringArm3D` |
| **Header** | `spark/ecs/components/camera/SpringArm3DComponent.hpp` |
| **2D** | 3D-only |

### `PolygonCollider2DComponent` {#polygoncollider2d}

| **Kind** | `ComponentKind::PolygonCollider2D` |
| **Header** | `spark/ecs/components/physics/2d/PolygonCollider2DComponent.hpp` |
| **2D** | Primary — convex static ramps (max 16 verts) |

```cpp
auto* poly = ramp->AddComponent<PolygonCollider2DComponent>();
poly->SetVertices(verts);
```

### `HealthComponent` {#health}

| **Kind** | `ComponentKind::Health` |
| **Header** | `spark/ecs/components/gameplay/HealthComponent.hpp` |
| **2D** | Primary |

```cpp
auto* hp = enemy->AddComponent<HealthComponent>(5.0F);
hp->SetOnDeath([](GameObject& self, GameObject* killer) {
    self.GetWorld().DestroyGameObject(&self);
});
```

### `DamageableComponent` {#damageable}

| **Kind** | `ComponentKind::Damageable` |
| **Header** | `spark/ecs/components/gameplay/DamageableComponent.hpp` |
| **2D** | Primary — damage multiplier / invuln hooks |

```cpp
enemy->AddComponent<DamageableComponent>()->SetDamageMultiplier(1.0F);
```

### `InteractableComponent` {#interactable}

| **Kind** | `ComponentKind::Interactable` |
| **Header** | `spark/ecs/components/gameplay/InteractableComponent.hpp` |
| **2D** | Primary |

```cpp
auto* chest = chestGo->AddComponent<InteractableComponent>();
chest->SetInteractionRadius(1.2F);
chest->SetOnInteract([](GameObject& player) { (void)player; });
```

### `PickupComponent` {#pickup}

| **Kind** | `ComponentKind::Pickup` |
| **Header** | `spark/ecs/components/gameplay/PickupComponent.hpp` |
| **2D** | Primary — pair with `TriggerVolume2D` |

```cpp
auto* pickup = gem->AddComponent<PickupComponent>();
pickup->SetItemId("gem");
pickup->SetAutoCollectOnTriggerEnter(true);
// After Simulate2D:
PickupComponent::ProcessDeferredDestroys(world);
```

### `AnimationHitbox2DComponent` {#animationhitbox2d}

| **Kind** | `ComponentKind::AnimationHitbox2D` |
| **Header** | `spark/ecs/components/animation/AnimationHitbox2DComponent.hpp` |
| **2D** | Primary — melee windows on sprite frames |

```cpp
auto* hitbox = player->AddComponent<AnimationHitbox2DComponent>();
hitbox->SetClipIndex(2U);
hitbox->SetShape(AnimationHitbox2DShape::Arc);
hitbox->SetRadius(1.1F);
hitbox->SetDamagePerHit(10.0F);
```

### `Hurtbox2DComponent` {#hurtbox2d}

| **Kind** | `ComponentKind::Hurtbox2D` |
| **Header** | `spark/ecs/components/physics/2d/Hurtbox2DComponent.hpp` |
| **2D** | Primary — trigger receiver for combat queries |

```cpp
enemy->AddComponent<Hurtbox2DComponent>()->SetRadius(0.68F);
enemy->AddComponent<HealthComponent>(3.0F);
```

### `Projectile2DComponent` {#projectile2d}

| **Kind** | `ComponentKind::Projectile2D` |
| **Header** | `spark/ecs/components/physics/2d/Projectile2DComponent.hpp` |
| **2D** | Primary |

```cpp
auto* shot = bulletGo->AddComponent<Projectile2DComponent>();
shot->SetDamage(1.0F);
shot->Activate({ox, oy}, {vx, vy}, owner);
Projectile2DComponent::ProcessDeferredDestroys(world);
```

### `DamageZone2DComponent` {#damagezone2d}

| **Kind** | `ComponentKind::DamageZone2D` |
| **Header** | `spark/ecs/components/gameplay/DamageZone2DComponent.hpp` |
| **2D** | Primary — lava, poison tiles |

```cpp
hazard->AddComponent<DamageZone2DComponent>()->SetDamagePerSecond(25.0F);
```

### `SpawnPoint2DComponent` {#spawnpoint2d}

| **Kind** | `ComponentKind::SpawnPoint2D` |
| **Header** | `spark/ecs/components/world/SpawnPoint2DComponent.hpp` |
| **2D** | Primary |

```cpp
spawn->AddComponent<SpawnPoint2DComponent>()->SetSpawnName("PlayerStart");
```

### `CameraBounds2DComponent` {#camerabounds2d}

| **Kind** | `ComponentKind::CameraBounds2D` |
| **Header** | `spark/ecs/components/camera/CameraBounds2DComponent.hpp` |
| **2D** | Primary — room camera boxes |

```cpp
zoneGo->AddComponent<CameraBounds2DComponent>()->SetHalfExtents({24.0F, 14.0F});
```

### `ParallaxLayerComponent` {#parallaxlayer}

| **Kind** | `ComponentKind::ParallaxLayer` |
| **Header** | `spark/ecs/components/rendering/ParallaxLayerComponent.hpp` |
| **2D** | Primary |

```cpp
auto* parallax = bg->AddComponent<ParallaxLayerComponent>();
parallax->SetFactorX(0.04F);
parallax->SetCameraReference(cameraGo);
```

### `ScreenShakeComponent` {#screenshake}

| **Kind** | `ComponentKind::ScreenShake` |
| **Header** | `spark/ecs/components/camera/ScreenShakeComponent.hpp` |
| **2D** | Primary — on camera rig with `Camera2DRig` |

```cpp
cameraGo->AddComponent<ScreenShakeComponent>()->AddImpulse({0.15F, -0.1F}, 0.2F, 30.0F);
```

### `SpriteAnimationEventReceiverComponent` {#spriteanimationeventreceiver}

| **Kind** | `ComponentKind::SpriteAnimationEventReceiver` |
| **Header** | `spark/ecs/components/animation/SpriteAnimationEventReceiverComponent.hpp` |
| **2D** | Primary — footstep markers |

```cpp
auto* recv = go->AddComponent<SpriteAnimationEventReceiverComponent>();
recv->AddMarker(1, 0.25F, "Footstep");
```

### `SpriteAnimationEventVfxComponent` {#spriteanimationeventvfx}

| **Kind** | `ComponentKind::SpriteAnimationEventVfx` |
| **Header** | `spark/ecs/components/animation/SpriteAnimationEventVfxComponent.hpp` |
| **2D** | Primary |

```cpp
go->AddComponent<SpriteAnimationEventVfxComponent>()->AddBinding("Footstep", "dust_puff");
```

### `InputActionMapComponent` {#inputactionmap}

| **Kind** | `ComponentKind::InputActionMap` |
| **Header** | `spark/ecs/components/input/InputActionMapComponent.hpp` |
| **2D** | Primary |

```cpp
auto* map = player->AddComponent<InputActionMapComponent>();
map->BindAxis1D("MoveX", GLFW_KEY_A, GLFW_KEY_D, GLFW_KEY_LEFT, GLFW_KEY_RIGHT);
map->BindButton("Jump", GLFW_KEY_SPACE);
```

### `PlayerInputComponent` {#playerinput}

| **Kind** | `ComponentKind::PlayerInput` |
| **Header** | `spark/ecs/components/input/PlayerInputComponent.hpp` |
| **2D** | Primary |

```cpp
auto* input = player->AddComponent<PlayerInputComponent>();
input->SetActionMap(map);
input->WasActionPressedThisFrame("Jump");
```

### `GameStateComponent` {#gamestate}

| **Kind** | `ComponentKind::GameState` |
| **Header** | `spark/ecs/components/gameplay/GameStateComponent.hpp` |
| **2D** | Primary |

```cpp
auto* flow = manager->AddComponent<GameStateComponent>(GameFlowState::Playing);
flow->RequestState(GameFlowState::Victory);
```

### `GameFlowTriggerComponent` {#gameflowtrigger}

| **Kind** | `ComponentKind::GameFlowTrigger` |
| **Header** | `spark/ecs/components/gameplay/GameFlowTriggerComponent.hpp` |
| **2D** | Primary — goal zones, death → defeat |

```cpp
goal->AddComponent<GameFlowTriggerComponent>();
goal->GetComponent<GameFlowTriggerComponent>()->SetTargetState(GameFlowState::Victory);
```

### `DecalProjectorComponent` {#decalprojector}

| **Kind** | `ComponentKind::DecalProjector` |
| **Header** | `spark/ecs/components/rendering/DecalProjectorComponent.hpp` |
| **2D** | 3D-only |

### `PhysicsMaterial2DComponent` {#physicsmaterial2d}

| **Kind** | `ComponentKind::PhysicsMaterial2D` |
| **Header** | `spark/ecs/components/physics/2d/PhysicsMaterial2DComponent.hpp` |
| **2D** | Primary — friction/restitution on static colliders |

```cpp
ice->AddComponent<PhysicsMaterial2DComponent>(0.15F, 0.35F);
```

### `MeshCollider3DComponent` {#meshcollider3d}

| **Kind** | `ComponentKind::MeshCollider3D` |
| **Header** | `spark/ecs/components/physics/3d/MeshCollider3DComponent.hpp` |
| **2D** | 3D-only |

### `NavMeshAgentComponent` {#navmeshagent}

| **Kind** | `ComponentKind::NavMeshAgent` |
| **Header** | `spark/ecs/components/ai/NavMeshAgentComponent.hpp` |
| **2D** | 3D-only — for tilemaps use `GridNavAgent2D` |

### `GridNavAgent2DComponent` {#gridnavagent2d}

| **Kind** | `ComponentKind::GridNavAgent2D` |
| **Header** | `spark/ecs/components/ai/GridNavAgent2DComponent.hpp` |
| **2D** | Primary |

```cpp
auto* nav = npc->AddComponent<GridNavAgent2DComponent>();
nav->SetGridSourceObject(mapGo);
nav->SetGoalMode(GridNavGoalMode2D::WorldPosition);
nav->SetGoalWorldPosition({gx, gy});
ProcessGridNavAgents2D(world, dt);
```

See [Pathfinding](../4-ai/05-pathfinding.md), [09 — Gameplay API](09-2d-gameplay-api-guide.md).

### `GridPathFollower2DComponent` {#gridpathfollower2d}

| **Kind** | `ComponentKind::GridPathFollower2D` |
| **Header** | `spark/ecs/components/ai/GridPathFollower2DComponent.hpp` |
| **2D** | Primary — moves transform or `Rigidbody2D` along path |

```cpp
player->AddComponent<GridPathFollower2DComponent>()->SetMaxSpeed(7.0F);
```

### `GridNavTarget2DComponent` {#gridnavtarget2d}

| **Kind** | `ComponentKind::GridNavTarget2D` |
| **Header** | `spark/ecs/components/ai/GridNavTarget2DComponent.hpp` |
| **2D** | Primary — chase target marker |

```cpp
player->AddComponent<GridNavTarget2DComponent>();
```

### `PatrolPathComponent` {#patrolpath}

| **Kind** | `ComponentKind::PatrolPath` |
| **Header** | `spark/ecs/components/ai/PatrolPathComponent.hpp` |
| **2D** | Optional — polyline waypoints (P0 demo uses grid goals + custom patrol) |

```cpp
auto* path = pathObj->AddComponent<PatrolPathComponent>();
path->GetWaypoints().PushBack({x0, y0, 0.0F});
path->SetLooping(true);
```

### `PerceptionSensorComponent` {#perceptionsensor}

| **Kind** | `ComponentKind::PerceptionSensor` |
| **Header** | `spark/ecs/components/ai/PerceptionSensorComponent.hpp` |
| **2D** | Primary — sight/hearing lists |

```cpp
guard->AddComponent<PerceptionSensorComponent>()->SetSightRadius(18.0F);
```

### `AmbientZoneComponent` {#ambientzone}

| **Kind** | `ComponentKind::AmbientZone` |
| **Header** | `spark/ecs/components/audio/AmbientZoneComponent.hpp` |
| **2D** | Primary |

```cpp
cave->AddComponent<AmbientZoneComponent>()->SetVolumeScale(0.7F);
```

### `FogVolumeComponent` {#fogvolume}

| **Kind** | `ComponentKind::FogVolume` |
| **Header** | `spark/ecs/components/rendering/FogVolumeComponent.hpp` |
| **2D** | 3D-only |

### `PostProcessVolumeComponent` {#postprocessvolume}

| **Kind** | `ComponentKind::PostProcessVolume` |
| **Header** | `spark/ecs/components/rendering/PostProcessVolumeComponent.hpp` |
| **2D** | 3D-only |

### `HingeJoint3DComponent` {#hingejoint3d}

| **Kind** | `ComponentKind::HingeJoint3D` |
| **Header** | `spark/ecs/components/physics/3d/HingeJoint3DComponent.hpp` |
| **2D** | 3D-only |

### `SpringJoint3DComponent` {#springjoint3d}

| **Kind** | `ComponentKind::SpringJoint3D` |
| **Header** | `spark/ecs/components/physics/3d/SpringJoint3DComponent.hpp` |
| **2D** | 3D-only |

### `DistanceJoint2DComponent` {#distancejoint2d}

| **Kind** | `ComponentKind::DistanceJoint2D` |
| **Header** | `spark/ecs/components/physics/2d/DistanceJoint2DComponent.hpp` |
| **2D** | Primary — ropes, chains |

```cpp
chainA->AddComponent<DistanceJoint2DComponent>(chainB, 1.2F);
```

### `HingeJoint2DComponent` {#hingejoint2d}

| **Kind** | `ComponentKind::HingeJoint2D` |
| **Header** | `spark/ecs/components/physics/2d/HingeJoint2DComponent.hpp` |
| **2D** | Primary — pinned doors |

```cpp
hingeA->AddComponent<HingeJoint2DComponent>(hingeB);
```

### `TilemapGameplayGridComponent` {#tilemapgameplaygrid}

| **Kind** | `ComponentKind::TilemapGameplayGrid` |
| **Header** | `spark/ecs/components/tilemap/TilemapGameplayGridComponent.hpp` |
| **2D** | Primary — walkability bake for A* |

```cpp
auto* grid = mapGo->AddComponent<TilemapGameplayGridComponent>();
grid->SetAutoRebake(true);
grid->RebakeIfNeeded(*mapGo);
```

### `TilemapTileAnimatorComponent` {#tilemaptileanimator}

| **Kind** | `ComponentKind::TilemapTileAnimator` |
| **Header** | `spark/ecs/components/tilemap/TilemapTileAnimatorComponent.hpp` |
| **2D** | Primary — animated tile cells |

Sibling on tilemap root; drives tileset animation time.

### `TilemapAutotileComponent` {#tilemapautotile}

| **Kind** | `ComponentKind::TilemapAutotile` |
| **Header** | `spark/ecs/components/tilemap/TilemapAutotileComponent.hpp` |
| **2D** | Primary — terrain autotile rebuild |

### `TilemapObjectLayerComponent` {#tilemapobjectlayer}

| **Kind** | `ComponentKind::TilemapObjectLayer` |
| **Header** | `spark/ecs/components/tilemap/TilemapObjectLayerComponent.hpp` |
| **2D** | Primary — Tiled object markers |

### `TilemapObjectSpawnComponent` {#tilemapobjectspawn}

| **Kind** | `ComponentKind::TilemapObjectSpawn` |
| **Header** | `spark/ecs/components/tilemap/TilemapObjectSpawnComponent.hpp` |
| **2D** | Primary — spawn registry from markers |

```cpp
mapGo->AddComponent<TilemapObjectSpawnComponent>()->SetRegistry(&spawnRegistry);
```

### `TilemapObjectGizmoComponent` {#tilemapobjectgizmo}

| **Kind** | `ComponentKind::TilemapObjectGizmo` |
| **Header** | `spark/ecs/components/tilemap/TilemapObjectGizmoComponent.hpp` |
| **2D** | Primary — debug draw for markers |

### `TilemapMapSourceComponent` {#tilemapmapsource}

| **Kind** | `ComponentKind::TilemapMapSource` |
| **Header** | `spark/ecs/components/tilemap/TilemapMapSourceComponent.hpp` |
| **2D** | Primary — `.tmx` / `.sparkmap` import |

```cpp
auto* source = mapGo->AddComponent<TilemapMapSourceComponent>();
source->SetTmxPath("level.tmx");
source->ImportNow(*mapGo, world);
```

### `TimeOfDayDriverComponent` {#timeofdaydriver}

| **Kind** | `ComponentKind::TimeOfDayDriver` |
| **Header** | `spark/ecs/components/world/TimeOfDayDriverComponent.hpp` |
| **2D** | Optional — sun color/time for lit sprites |

```cpp
root->AddComponent<TimeOfDayDriverComponent>()->SetTimeOfDay(0.35F);
```

### `SpawnPointComponent` {#spawnpoint}

| **Kind** | `ComponentKind::SpawnPoint` |
| **Header** | `spark/ecs/components/world/SpawnPointComponent.hpp` |
| **2D** | 3D-only — use `SpawnPoint2D` |

### `GltfSceneSourceComponent` {#gltfscenesource}

| **Kind** | `ComponentKind::GltfSceneSource` |
| **Header** | `spark/ecs/components/rendering/GltfSceneSourceComponent.hpp` |
| **2D** | 3D-only — serialized glTF scene root |

### `GltfInstanceNodeComponent` {#gltfinstancenode}

| **Kind** | `ComponentKind::GltfInstanceNode` |
| **Header** | `spark/ecs/components/rendering/GltfInstanceNodeComponent.hpp` |
| **2D** | 3D-only — per-node glTF instance data |

### `WaterBodyComponent` {#waterbody}

| **Kind** | `ComponentKind::WaterBody` |
| **Header** | `spark/ecs/components/water/WaterBodyComponent.hpp` |
| **2D** | 3D-only |

### `PointLight2DComponent` {#pointlight2d}

| **Kind** | `ComponentKind::PointLight2D` |
| **Header** | `spark/ecs/components/lighting/PointLight2DComponent.hpp` |
| **2D** | Primary — local lamp on sprites |

```cpp
lamp->AddComponent<PointLight2DComponent>(Vector3{1,0.9,0.7}, 2.0F, 8.0F);
```

### `Scene2DCompositeViewComponent` {#scene2dcompositeview}

| **Kind** | `ComponentKind::Scene2DCompositeView` |
| **Header** | `spark/ecs/components/rendering/Scene2DCompositeViewComponent.hpp` |
| **2D** | Primary — GPU minimap / composite HUD |

```cpp
auto* view = flow->AddComponent<Scene2DCompositeViewComponent>();
view->SetFeature(Scene2DCompositeFeature::Minimap);
view->SetTarget(minimapRenderTexture);
```

See [08 — Runtime limits](08-scene2d-runtime-limits.md).

---

## Checklist: platformer entity

| Object | `ComponentKind` stack |
|--------|------------------------|
| Player | `Transform`, `Sprite`, `Sprite2DCharacterAnimFsm`, `SpriteAnimator`, `AnimationHitbox2D`, `BoxCollider2D`, `Rigidbody2D`, `CharacterController2D`, `InputActionMap`, `PlayerInput`, `Health`, `Damageable` |
| Enemy | `Transform`, `Sprite`, `Hurtbox2D`, `Health`, `AiAgent` |
| Gem | `Transform`, `Sprite`, `SpriteLighting2D`, `TriggerVolume2D`, `Pickup` |
| Goal | `Transform`, `TriggerVolume2D`, `GameFlowTrigger`, `GameState` (on manager) |
| Camera | `Camera2D`, `Camera2DRig`, `ScreenShake`, `AudioListener` |

## Checklist: tilemap / P0 path

| Object | `ComponentKind` stack |
|--------|------------------------|
| Map | `Tilemap`, `TilemapMapSource`, `TilemapGameplayGrid`, `TilemapCollider2D`, `TilemapObjectLayer`, `TilemapObjectSpawn` |
| Player | `GridNavAgent2D`, `GridPathFollower2D`, `Rigidbody2D`, `Sprite`, … |
| NPC | `GridNavAgent2D`, `Rigidbody2D`, `AiAgent` (optional) |
| HUD | `Scene2DCompositeView`, `TextOverlay` |

Next: [Part 7 overview](00-2d-game-systems-map.md) · [Game component reference](../1-overview-architecture/07-game-component-reference.md).
