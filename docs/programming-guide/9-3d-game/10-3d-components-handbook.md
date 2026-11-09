# 3D components handbook

One page **oriented at 3D action / FPS / open slices** — mirror of [2D components handbook](../8-2d-game/10-2d-components-handbook.md), which still lists **all 120** kinds in enum order.

**Field API:** [Game component reference](../1-overview-architecture/07-game-component-reference.md) · **Attach recipes:** [Part 7](../7-game-components-usage/02-3d-rendering.md).

**Include:** `#include "spark/ecs/Ecs.hpp"` or per-header under `spark/ecs/components/`.

### Quick jumps (3D stacks)

| Topic | Anchor |
|-------|--------|
| Draw / materials | [#mesh](#mesh) · [#material](#material) · [#skinnedmesh](#skinnedmesh) |
| Animation | [#animator](#animator) · [#character3danimfsm](#character3danimfsm) · [#rootmotion](#rootmotion) |
| Camera / character | [#charactercontroller3d](#charactercontroller3d) · [#camerafollow3d](#camerafollow3d) · [#springarm3d](#springarm3d) · [#character3dbody](#character3dbody) |
| Physics 3D | [#boxcollider3d](#boxcollider3d) · [#rigidbody3d](#rigidbody3d) · [#triggervolume3d](#triggervolume3d) · [#physicslayer3d](#physicslayer3d) |
| World / env | [#terrain](#terrain) · [#waterbody](#waterbody) · [#timeofdaydriver](#timeofdaydriver) · [#fogvolume](#fogvolume) |
| glTF | [#gltfscenesource](#gltfscenesource) |
| AI 3D | [#navmeshagent](#navmeshagent) · [#perceptionsensor](#perceptionsensor) |

---

## Master index (3D angle)

| Kind | Component | 3D role |
|------|-----------|---------|
| `Transform` | `TransformComponent` | **Primary** — all 3D entities |
| `Mesh` | `MeshComponent` | **Primary** — static draws |
| `Material` | `MaterialComponent` | **Primary** — PBR/toon |
| `MultiMaterial` | `MultiMaterialComponent` | glTF multi-slot |
| `SkinnedMesh` | `SkinnedMeshComponent` | Characters |
| `Animator` | `AnimatorComponent` | Skeletal playback |
| `Sky` | `SkyComponent` | Outdoor backgrounds |
| `Terrain` | `TerrainComponent` | Heightfields |
| `WaterBody` | `WaterBodyComponent` | Lakes/ocean |
| `ParticleEmitter` | `ParticleEmitterComponent` | Optional FX |
| `VfxPlayer` | `VfxPlayerComponent` | `.sparkvfx` bursts |
| `DirectionalLight` | `DirectionalLightComponent` | Sun |
| `PointLight` / `SpotLight` | punctual lights | Torches, lamps |
| `Camera` | `CameraComponent` | 3D projection rig |
| `CameraFollow3D` | `CameraFollow3DComponent` | Chase cameras |
| `SpringArm3D` | `SpringArm3DComponent` | Shoulder offset |
| `BoxCollider3D` … `CapsuleCollider3D` | colliders | Physics |
| `Rigidbody3D` | `Rigidbody3DComponent` | Dynamic props |
| `CharacterController3D` | `CharacterController3DComponent` | FPS motor |
| `TriggerVolume3D` | `TriggerVolume3DComponent` | Goals, zones |
| `MeshCollider3D` | `MeshCollider3DComponent` | Static triangle mesh |
| `PhysicsLayer3D` | `PhysicsLayer3DComponent` | Layer filter |
| `PhysicsContactListener3D` | contact listener | Impact gameplay |
| `PhysicsAttachedBody3D` | carry handoff | Showcase **P** |
| `Character3DBody` / `Character3DVisual` | motor + visual child | Character **#5** |
| `Character3DAnimFsm` | combat/loco FSM | Action games |
| `AnimationLayerStack` | upper-body overlay | Attacks while moving |
| `RootMotion` | `RootMotionComponent` | Attack lunges |
| `FootIk` / `AimIk` | IK at submit | Grounding, aim |
| `FogVolume` / `PostProcessVolume` | regional render | Mood, caves |
| `TimeOfDayDriver` | sun cycle | **#17** |
| `FoliageInstancedMesh` / `GrassField` | foliage | **#27–28** |
| `GltfSceneSource` / `GltfInstanceNode` | imported scenes | **#21** |
| `NavMeshAgent` / `PerceptionSensor` | guards | **#8** |
| `SpawnPoint` | spawn metadata | Scene markers |
| `DecalProjector` | projected decals | Optional |
| `Billboard` | camera-facing quads | Optional FX |

**2D-only kinds** (sprites, tilemaps, `CharacterController2D`, …): see [2D handbook](../8-2d-game/10-2d-components-handbook.md).

**Shared gameplay** (`Health`, `Damageable`, `PlayerInput`, `GameState`, `SoundCue`, `AudioListener`, `AiAgent`): same stacks in 3D — field reference + Part 7.

---

## Catalog (3D-focused)

### `TransformComponent` {#transform}

| **Kind** | `ComponentKind::Transform` |
| **Header** | `spark/ecs/components/core/TransformComponent.hpp` |

```cpp
tr->SetTranslation({x, y, z});
tr->SetRotation(Quaternion::FromEulerY(yaw));
tr->SetUniformScale(1.0F);
```

### `MeshComponent` {#mesh}

| **Kind** | `ComponentKind::Mesh` |
| **Header** | `spark/ecs/components/rendering/MeshComponent.hpp` |

```cpp
go->AddComponent<MeshComponent>(meshAsset, SceneMeshSlot::Custom, tint);
```

Register assets: `world.RegisterMesh(sharedMesh, key)`.

### `MaterialComponent` {#material}

| **Kind** | `ComponentKind::Material` |
| **Header** | `spark/ecs/components/rendering/MaterialComponent.hpp` |

```cpp
auto* mat = go->AddComponent<MaterialComponent>();
mat->SetRoughness(0.4F);
mat->SetBaseColorTexture(albedo);
mat->SetEmissive({0.2F, 0.9F, 1.0F}, 2.0F);
```

### `SkinnedMeshComponent` {#skinnedmesh}

| **Kind** | `ComponentKind::SkinnedMesh` |
| **Header** | `spark/ecs/components/rendering/SkinnedMeshComponent.hpp` |

Pair with `AnimatorComponent` + skeleton asset. See [Skinned characters](../3-3d-graphics/05-skinned-characters.md).

### `AnimatorComponent` {#animator}

| **Kind** | `ComponentKind::Animator` |
| **Header** | `spark/ecs/components/animation/AnimatorComponent.hpp` |

Priority **200** — runs after animation drivers (100). Locomotion blend: `SetLocomotionBlend`.

### `Character3DAnimFsmComponent` {#character3danimfsm}

| **Kind** | `ComponentKind::Character3DAnimFsm` |
| **Header** | `spark/ecs/components/animation/Character3DAnimFsmComponent.hpp` |

```cpp
auto* fsm = body->AddComponent<Character3DAnimFsmComponent>();
fsm->RequestAttack();
```

Blackboard slot: `kAiBlackboardIntCharacter3DCombatCommand`.

### `AnimationLayerStackComponent` {#animationlayerstack}

| **Kind** | `ComponentKind::AnimationLayerStack` |
| **Header** | `spark/ecs/components/animation/AnimationLayerStackComponent.hpp` |

Upper-body overlay while locomotion runs on base animator.

### `RootMotionComponent` {#rootmotion}

| **Kind** | `ComponentKind::RootMotion` |
| **Header** | `spark/ecs/components/animation/RootMotionComponent.hpp` |

Feed into `Character3DBody` motor arbitration — demo **#5**.

### `TerrainComponent` {#terrain}

| **Kind** | `ComponentKind::Terrain` |
| **Header** | `spark/ecs/components/rendering/TerrainComponent.hpp` |

Sculpt + `RegenerateMesh` — [Terrain & sky](../3-3d-graphics/06-terrain-and-sky.md).

### `WaterBodyComponent` {#waterbody}

| **Kind** | `ComponentKind::WaterBody` |
| **Header** | `spark/ecs/components/water/WaterBodyComponent.hpp` |

```cpp
go->AddComponent<WaterBodyComponent>(
    WaterBodyMode::InfiniteOcean, 0.0F, extent, preset, meshSettings, surfaceColor);
```

[Environment stack](../3-3d-graphics/09-environment-stack.md).

### `SkyComponent` {#sky}

| **Kind** | `ComponentKind::Sky` |
| **Header** | `spark/ecs/components/rendering/SkyComponent.hpp` |

`SceneSkyMode::Dome` + HDR texture optional.

### `DirectionalLightComponent` {#directionallight}

| **Kind** | `ComponentKind::DirectionalLight` |
| **Header** | `spark/ecs/components/lighting/DirectionalLightComponent.hpp` |

Sun for CSM — [Lighting](../3-3d-graphics/03-lighting.md).

### `CharacterController3DComponent` {#charactercontroller3d}

| **Kind** | `ComponentKind::CharacterController3D` |
| **Header** | `spark/ecs/components/physics/3d/CharacterController3DComponent.hpp` |

```cpp
auto* cc = player->AddComponent<CharacterController3DComponent>();
cc->SetHeight(1.8F);
cc->SetRadius(0.35F);
```

Motor via `PhysicsSubsystem::GetCharacterController3D()` — not a full `Rigidbody3D`.

### `Character3DBodyComponent` {#character3dbody}

| **Kind** | `ComponentKind::Character3DBody` |
| **Header** | `spark/ecs/components/character/Character3DBodyComponent.hpp` |

Physics root + `MotionArbitrator` + motor. Child visual uses `Character3DVisualComponent`.

### `CameraFollow3DComponent` {#camerafollow3d}

| **Kind** | `ComponentKind::CameraFollow3D` |
| **Header** | `spark/ecs/components/camera/CameraFollow3DComponent.hpp` |

Follow target transform each frame — [Character camera](07-character-and-third-person.md).

### `SpringArm3DComponent` {#springarm3d}

| **Kind** | `ComponentKind::SpringArm3D` |
| **Header** | `spark/ecs/components/camera/SpringArm3DComponent.hpp` |

Collision-aware camera offset; pairs with `CameraComponent` on child.

### `BoxCollider3DComponent` {#boxcollider3d}

| **Kind** | `ComponentKind::BoxCollider3D` |
| **Header** | `spark/ecs/components/physics/3d/BoxCollider3DComponent.hpp` |

```cpp
box->SetCategoryBits(CollisionFilter3D::LayerBit(PhysicsLayers3D::Environment));
box->SetMaskBits(CollisionFilter3D::AllLayersMask());
```

[Layers](../5-physics/08-collision-layers-and-filters.md).

### `Rigidbody3DComponent` {#rigidbody3d}

| **Kind** | `ComponentKind::Rigidbody3D` |
| **Header** | `spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp` |

Dynamic spheres/capsules — minimal solver; see [Physics 3D](../5-physics/05-physics-3d.md).

### `TriggerVolume3DComponent` {#triggervolume3d}

| **Kind** | `ComponentKind::TriggerVolume3D` |
| **Header** | `spark/ecs/components/physics/3d/TriggerVolume3DComponent.hpp` |

Enter/stay/exit — [Triggers](../5-physics/09-triggers-contacts-and-showcase.md).

### `PhysicsLayer3DComponent` {#physicslayer3d}

| **Kind** | `ComponentKind::PhysicsLayer3D` |
| **Header** | `spark/ecs/components/physics/3d/PhysicsLayer3DComponent.hpp` |

Overrides category/mask for all colliders on object.

### `FogVolumeComponent` {#fogvolume}

| **Kind** | `ComponentKind::FogVolume` |
| **Header** | `spark/ecs/components/rendering/FogVolumeComponent.hpp` |

[Post-process & volumes](../3-3d-graphics/10-post-process-fog-volumes.md).

### `TimeOfDayDriverComponent` {#timeofdaydriver}

| **Kind** | `ComponentKind::TimeOfDayDriver` |
| **Header** | `spark/ecs/components/world/TimeOfDayDriverComponent.hpp` |

```cpp
driver->SetDayLengthSeconds(120.0F);
driver->SetTimeOfDay(0.35F);
```

### `GltfSceneSourceComponent` {#gltfscenesource}

| **Kind** | `ComponentKind::GltfSceneSource` |
| **Header** | `spark/ecs/components/rendering/GltfSceneSourceComponent.hpp` |

Root for imported glTF hierarchy — pump assets before instantiate.

### `NavMeshAgentComponent` {#navmeshagent}

| **Kind** | `ComponentKind::NavMeshAgent` |
| **Header** | `spark/ecs/components/ai/NavMeshAgentComponent.hpp` |

Patrol polyline → `AiAgent` path — [Perception & nav](../4-ai/06-perception-nav-and-blackboard.md).

### `PerceptionSensorComponent` {#perceptionsensor}

| **Kind** | `ComponentKind::PerceptionSensor` |
| **Header** | `spark/ecs/components/ai/PerceptionSensorComponent.hpp` |

```cpp
sensor->SetSightRadius(10.0F);
sensor->SetSightFovDegrees(120.0F);
```

### `FoliageInstancedMeshComponent` {#foliageinstancedmesh}

| **Kind** | `ComponentKind::FoliageInstancedMesh` |
| **Header** | `spark/ecs/components/foliage/FoliageInstancedMeshComponent.hpp` |

Wind-driven instances — demo **#27**.

### `GrassFieldComponent` {#grassfield}

| **Kind** | `ComponentKind::GrassField` |
| **Header** | `spark/ecs/components/foliage/GrassFieldComponent.hpp` |

```cpp
field->SetTerrainObject(terrainGo);
field->GetBounds().SetHalfExtentsMeters(40.0F, 40.0F);
```

### `ParticleEmitterComponent` / `VfxPlayerComponent` {#vfx}

Emitters for local FX; `VfxPlayer` plays `.sparkvfx` assets — [VFX chapter](../3-3d-graphics/08-vfx-and-sparkvfx.md).

---

## Checklist: minimal FPS arena

| Object | Stack |
|--------|--------|
| Player | `Transform`, `CharacterController3D`, `Camera` (or fly cam in code), `AudioListener`, `PlayerInput` |
| Ground | `Mesh` + `Material`, `BoxCollider3D` static |
| Target | `Mesh`, `Health`, `Damageable`, `Hurtbox3D` |
| Sun | `DirectionalLight`, `Transform` aimed dir |
| Manager | `GameState`, physics owned by `Game` |

Walkthrough: [FPS intro](01-fps-intro.md).

## Checklist: third-person character

| Object | Stack |
|--------|--------|
| Body | `Character3DBody`, `CharacterController3D`, `CapsuleCollider3D` |
| Visual | `Character3DVisual`, child `SkinnedMesh`, `Animator`, `Character3DAnimFsm`, `AnimationLayerStack` |
| Camera rig | `SpringArm3D`, `CameraFollow3D`, `Camera` |
| Optional | `FootIk`, `AimIk`, `RootMotion` |

[Character & third person](07-character-and-third-person.md).

## Checklist: outdoor environment slice

| Object | Stack |
|--------|--------|
| Terrain | `Terrain`, `Material` |
| Sky/TOD | `Sky`, `TimeOfDayDriver` |
| Water | `WaterBody` |
| Grass | `GrassField` bound to terrain |
| Post | `FogVolume`, `PostProcessVolume` |
| Submit | outdoor profile + extended `shadowCascadeFar` |

[Environment stack](../3-3d-graphics/09-environment-stack.md).

---

Next: [3D systems map](00-3d-game-systems-map.md) · [2D handbook](../8-2d-game/10-2d-components-handbook.md) · [Roadmap hub](../1-overview-architecture/16-glossary-and-roadmap-hub.md).
