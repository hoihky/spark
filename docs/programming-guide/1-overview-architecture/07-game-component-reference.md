# Game Component Reference

Complete reference for all **70** built-in `GameComponent` types in Spark (`include/spark/ecs/components/`). Every component has exactly one `ComponentKind` value; lookup uses `GetComponent<T>()` which matches `T::TypeKind`.

**Includes:** `#include "spark/ecs/Ecs.hpp"` (umbrella) or the specific header under `spark/ecs/components/`.

**Subsystem tick order (typical frame):**

1. `UpdateGameObjects` — component `OnUpdate` (billboards, springs, …)
2. `PhysicsSubsystem::Simulate2D` / `SimulateAll3D` (+ character controllers, triggers)
3. `SimulateGameAi` — `ProcessNavMeshAgents`, `ProcessPerceptionSensors`, then `AiAgentComponent`
4. `ProcessSoundCues` — `ProcessAudioListeners`, `ProcessAmbientZones`, cue flush
5. `FillStandardLitSceneFromWorld` — `ProcessTimeOfDayDrivers`, regional fog/post volumes, lighting resolve

---

## Quick index

| Folder | Components |
|--------|------------|
| [Core](#core) | `TransformComponent` |
| [Rendering](#rendering) | `Mesh`, `Material`, `SkinnedMesh`, `Sprite`, `Tilemap`, `Sky`, `Terrain`, `ParticleEmitter`, `TextOverlay`, `Billboard`, `DecalProjector`, `FogVolume`, `PostProcessVolume`, `BlendMode`, `RenderLayer`, `SortingGroup`, `SpriteLighting2D` |
| [Tilemap](#tilemap) | `TilemapGameplayGrid`, `TilemapTileAnimator`, `TilemapAutotile`, `TilemapObjectLayer`, `TilemapObjectSpawn`, `TilemapObjectGizmo`, `TilemapMapSource` |
| [Lighting](#lighting) | `DirectionalLight`, `PointLight`, `SpotLight` |
| [Camera](#camera) | `Camera`, `Camera2D`, `Camera2DRig`, `CameraFollow3D`, `SpringArm3D` |
| [Physics 2D](#physics-2d) | `BoxCollider2D`, `CircleCollider2D`, `PolygonCollider2D`, `Rigidbody2D`, `TilemapCollider2D`, `PhysicsMaterial2D`, `DistanceJoint2D`, `HingeJoint2D` |
| [Physics 3D](#physics-3d) | `BoxCollider3D`, `SphereCollider3D`, `CapsuleCollider3D`, `MeshCollider3D`, `Rigidbody3D`, `CharacterController3D`, `TriggerVolume3D`, `PhysicsMaterial3D`, `DistanceJoint3D`, `HingeJoint3D`, `SpringJoint3D`, `Collision` |
| [Animation](#animation) | `Animator`, `SpriteAnimator`, `AnimationEventReceiver`, `AttachmentSocket`, `Character3DAnimFsm`, `Sprite2DCharacterAnimFsm` |
| [AI](#ai) | `AiAgent`, `NavMeshAgent`, `PatrolPath`, `PerceptionSensor` |
| [Audio](#audio) | `SoundCue`, `AudioListener`, `AmbientZone` |
| [UI](#ui) | `UiCanvas` |
| [World](#world) | `SceneSpatialPolicy`, `TimeOfDayDriver` |
| [Gameplay](#gameplay) | `Health`, `Damageable` |

---

## Update priorities

| Priority | Constant / note | Components |
|----------|-----------------|------------|
| 0 | default | Most components; `ParticleEmitterComponent` |
| 50 | — | `BillboardComponent` |
| 100 | `ComponentUpdatePriority::AnimationDriver` | `Character3DAnimFsmComponent`, `Sprite2DCharacterAnimFsmComponent` |
| 200 | `ComponentUpdatePriority::AnimatorPlayback` | `AnimatorComponent`, `SpriteAnimatorComponent` |
| 210 | — | `AnimationEventReceiverComponent` |
| 250 | — | `AttachmentSocketComponent` |
| 295 | — | `SpringArm3DComponent` |
| 300 | — | `Camera2DRigComponent`, `CameraFollow3DComponent` |

**Rule of thumb:** add animation **drivers** (FSM) before **playback** (`Animator` / `SpriteAnimator`) on the same `GameObject`.

---

## Signals (`spark/ecs/Signal.hpp`)

| `SignalId` | Payload | Emitted by |
|------------|---------|------------|
| `TransformChanged` | — | `TransformComponent` on TRS change |
| `MeshDirty` | — | mesh/material paths |
| `CollisionBoundsDirty` | — | `CollisionComponent` |
| `Physics2DTriggerOverlap` | `ptr` = other `GameObject*`, `a` = id, `b` = static index | `PhysicsWorld2D` |
| `Physics3DTriggerEnter` / `Exit` | `ptr` = other `GameObject*`, `a` = id | `TriggerVolumeWorld3D` |
| `AnimationEvent` | `ptr` = event name C-string, `a` = clip index | `AnimationEventReceiverComponent` |
| `DamageApplied` | `ptr` = `DamageSignalPayload*` (sync only) | `HealthComponent` |
| `Died` | `ptr` = instigator `GameObject*` or null | `HealthComponent` |
| `UserBase + n` | custom | your gameplay code |

```cpp
void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override {
    if (id == SignalId::Physics2DTriggerOverlap) {
        GameObject* other = static_cast<GameObject*>(payload.ptr);
        (void)other;
    }
    if (id == SignalId::AnimationEvent) {
        const char* name = static_cast<const char*>(payload.ptr);
        (void)name;
    }
    if (id == SignalId::DamageApplied) {
        const auto* dmg = static_cast<const DamageSignalPayload*>(payload.ptr);
        (void)dmg;
    }
}
```

---

## Core

### `TransformComponent`

**Header:** `spark/ecs/components/core/TransformComponent.hpp`  
**Kind:** `ComponentKind::Transform`  
**Consumed by:** Everything that needs pose (render, physics, audio, cameras).

Local translation / rotation / quaternion scale. Parent chain composes `GameObject::GetWorldMatrix()`.

```cpp
GameObject* player = world.CreateGameObject();
auto* tr = player->AddComponent<TransformComponent>();
tr->SetTranslation({0.0F, 1.0F, 0.0F});
tr->SetUniformScale(1.5F);
tr->SetRotation(Quaternion::FromAxisAngle(Vector3::UnitY, 0.5F));
```

---

## Rendering

### `MeshComponent`

**Kind:** `Mesh` · **Consumed by:** `SubmitStandardLitSceneFromWorld`

Static triangle mesh + optional `SceneMeshSlot` (GroundPlane, SkyBox, Custom, …).

```cpp
SharedPtr<Mesh> cube = world.LoadMesh("assets/models/cube.obj");
auto* mesh = go->AddComponent<MeshComponent>(cube, SceneMeshSlot::Custom, Vector3::One);
mesh->SetAlbedo({0.8F, 0.2F, 0.2F});
```

### `MaterialComponent`

**Kind:** `Material` · **Consumed by:** Scene submit (PBR / toon, transparency partition)

```cpp
SharedPtr<Texture2D> albedo = world.LoadTexture("assets/brick.png");
go->AddComponent<MaterialComponent>(albedo, Vector3::One);
auto* mat = go->GetComponent<MaterialComponent>();
mat->SetMetallic(0.1F);
mat->SetRoughness(0.7F);
mat->SetNormalTexture(normalMap);
mat->SetMetallicRoughnessTexture(ormMap);
mat->SetOpacity(0.85F);  // → transparent pass
```

### `MultiMaterialComponent`

**Kind:** `Unknown` (serialization pending) · **Sibling:** `MeshComponent` on multi-material glTF meshes

Per-submesh material slots indexed by `MeshSubmesh::materialIndex`. Populated from a loaded glTF asset:

```cpp
GltfAsset asset = world.LoadGltf("assets/models/Building.glb");
go->AddComponent<MeshComponent>(asset.mesh, SceneMeshSlot::Custom, Vector3::One);
auto* multi = go->AddComponent<MultiMaterialComponent>();
multi->PopulateFromGltfAsset(asset);
// Scene submit reads slots when partitioning draws per submesh.
```

### `SkinnedMeshComponent` + `AnimatorComponent`

**Kinds:** `SkinnedMesh`, `Animator` · **Consumed by:** Scene submit (joint palette)

```cpp
SkinnedGltfAsset fox = world.LoadSkinnedGltf("assets/Fox.glb");
go->AddComponent<SkinnedMeshComponent>(fox.mesh);
go->AddComponent<MaterialComponent>(fox.baseColorTexture);
auto* anim = go->AddComponent<AnimatorComponent>(fox.skeleton, 0, 1.0F);
anim->SetLoopMode(AnimLoopMode::Loop);
anim->SetClipIndexWithCrossfade(1, 0.25F);
```

### `SpriteComponent`

**Kind:** `Sprite` · **Consumed by:** Sprite pass in `SceneRenderParams`

```cpp
SharedPtr<Texture2D> sheet = world.LoadTexture("assets/hero.png");
auto* sprite = go->AddComponent<SpriteComponent>(sheet, Vector4{1,1,1,1}, Vector4{0,0,1,1}, 10);
sprite->SetSortOrder(5);  // higher draws on top within layer
```

### `SpriteLighting2DComponent`

**Kind:** `SpriteLighting2D` · **Sibling:** `SpriteComponent`

```cpp
go->AddComponent<SpriteLighting2DComponent>(SpriteLighting2DMode::NormalMapped, 1.0F, 0.0F);
```

### `SpriteAnimatorComponent`

**Kind:** `SpriteAnimator` · **Priority:** 200 · **Sibling:** `SpriteComponent`

```cpp
auto* sa = go->AddComponent<SpriteAnimatorComponent>();
sa->SetUniformGrid(4, 4);
SpriteAnimationClip idle{};
idle.firstFrame = 0;
idle.frameCount = 4;
idle.framesPerSecond = 8.0F;
sa->AddClip(idle);
sa->SetClipIndex(0);
```

### `TilemapComponent`

**Kind:** `Tilemap` · **Consumed by:** Tilemap pass · **Guide:** [Tilemaps](../2-2d-graphics/03-tilemaps.md)

Multi-layer grid; each `TilemapLayer` holds `TileCell` data. Rendering uses `Tileset` atlas layout (including Tiled margin/spacing when set).

```cpp
SharedPtr<Tileset> set = CreateTilesetFromAtlas(atlas, 12, 11);
auto* tm = go->AddComponent<TilemapComponent>(set, 32, 20, 1.0F, 0);
static_cast<void>(tm->AddLayer("Dungeon"));
tm->SetPaintTile(0, x, y, floorPaintId);
tm->BakeGameplayGrid(grid, TilemapGameplayWalkRule::DefinitionAndFlags);
```

### `TilemapCollider2DComponent`

**Kind:** `TilemapCollider2D` · **Sibling:** `TilemapComponent` · **Consumed by:** `PhysicsWorld2D`

Bakes static colliders from `TileDefinition` on layers with `contributeCollision`.

```cpp
auto* tc = go->AddComponent<TilemapCollider2DComponent>();
tc->SetCategoryBits(1u << 1);
tc->SetMaskBits(0xFFFF);
```

---

## Tilemap

### `TilemapGameplayGridComponent`

**Kind:** `TilemapGameplayGrid` · **Sibling:** `TilemapComponent`

Caches a walkability grid and `TilemapGridFrame` for pathfinding / gameplay queries.

```cpp
auto* grid = go->AddComponent<TilemapGameplayGridComponent>();
grid->SetWalkRule(TilemapGameplayWalkRule::DefinitionAndFlags);
grid->SetAutoRebake(true);
grid->RebakeIfNeeded(*go);
```

### `TilemapMapSourceComponent`

**Kind:** `TilemapMapSource` · **Sibling:** `TilemapComponent`

Imports `.tmx` or `.sparkmap` via `ImportNow`, optional hot reload. Sets `pixelsPerWorldUnit` on `TilemapDocumentApplyOptions`.

```cpp
auto* source = go->AddComponent<TilemapMapSourceComponent>();
source->SetTmxPath("sprites/kenney_tiny-dungeon/Tiled/sampleMap.tmx");
source->SetPixelsPerWorldUnit(16.0F);
source->ImportNow(*go, world);
```

### `TilemapAutotileComponent` / `TilemapTileAnimatorComponent`

**Kinds:** `TilemapAutotile`, `TilemapTileAnimator` · **Sibling:** `TilemapComponent`

Autotile rebuild from painted terrain groups; global time for tile animation clips on the tileset.

### `TilemapObjectLayerComponent` / `TilemapObjectSpawnComponent` / `TilemapObjectGizmoComponent`

**Kinds:** `TilemapObjectLayer`, `TilemapObjectSpawn`, `TilemapObjectGizmo`

Tiled-style markers (cell + offset), registry-based spawning (`TilemapObjectSpawnRegistry`), and debug gizmo sprites.

---

### `SkyComponent`

**Kind:** `Sky` · **Siblings:** `MeshComponent` (sky mesh), `TransformComponent`

```cpp
go->AddComponent<MeshComponent>(skyBoxMesh, SceneMeshSlot::Custom);
go->AddComponent<SkyComponent>(SceneSkyMode::Box);
```

### `TerrainComponent`

**Kind:** `Terrain` · **Consumed by:** Mesh rebuild + raycast

```cpp
TerrainGeneratorSettings settings{};
settings.subdivX = 128;
settings.subdivZ = 128;
settings.halfExtentX = 56.0F;
settings.halfExtentZ = 56.0F;
settings.heightScale = 14.0F;
settings.noiseScale = 0.055F;
go->AddComponent<TerrainComponent>(settings, Vector3{0.4F, 0.55F, 0.35F});
go->AddComponent<MaterialComponent>(grassTex);
```

### `ParticleEmitterComponent`

**Kind:** `ParticleEmitter` · **Consumed by:** `OnUpdate` + `SceneRenderParams::particles`

```cpp
auto* pe = go->AddComponent<ParticleEmitterComponent>();
pe->SetEmitterEnabled(true);
pe->SetMaxParticles(256);
pe->SetEmissionRate(40.0F);
pe->SetStartEndColor({1.0F, 0.6F, 0.1F, 1.0F}, {0.85F, 0.05F, 0.0F, 0.0F});
pe->SetGravity({0.0F, -4.0F, 0.0F});
```

### `BillboardComponent`

**Kind:** `Billboard` · **Priority:** 50 · **Consumed by:** `OnUpdate` (rotates transform)

```cpp
go->AddComponent<SpriteComponent>(iconTex);
go->AddComponent<BillboardComponent>();  // default: full camera facing
// YAxisLocked for trees / pickups in 3D:
auto* bb = go->AddComponent<BillboardComponent>();
bb->SetMode(BillboardMode::YAxisLocked);
```

### `DecalProjectorComponent`

**Kind:** `DecalProjector` · **Consumed by:** `SceneRenderParams::decals` (data path; GPU decal pass optional)

```cpp
auto* decal = go->AddComponent<DecalProjectorComponent>();
decal->SetTexture(bulletHoleTex);
decal->SetSize({0.4F, 0.4F, 0.2F});
decal->SetOpacity(0.9F);
```

### `FogVolumeComponent`

**Kind:** `FogVolume` · **Consumed by:** `ApplyRegionalRenderVolumes` → `SceneRenderParams` (data path; GPU fog optional)

Oriented box or sphere volume. When the camera is inside, overrides `fogEnabled`, `fogColor`, `fogDensity`, `fogStart`, `fogEnd`. Higher `priority` wins overlaps.

```cpp
auto* fog = cave->AddComponent<FogVolumeComponent>();
fog->SetShape(VolumeShape::Box);
fog->SetHalfExtents({12.0F, 6.0F, 12.0F});
fog->SetFogColor({0.55F, 0.58F, 0.62F});
fog->SetFogDensity(0.035F);
fog->SetPriority(5);
```

### `PostProcessVolumeComponent`

**Kind:** `PostProcessVolume` · **Consumed by:** `ApplyRegionalRenderVolumes`

Regional overrides for SSAO, exposure, and ambient scale when the camera is inside the volume.

```cpp
auto* pp = interior->AddComponent<PostProcessVolumeComponent>();
pp->SetSsaoEnabled(false);
pp->SetExposure(1.15F);
pp->SetAmbientScale(0.85F);
pp->SetPriority(2);
```

### `TextOverlayComponent`

**Kind:** `TextOverlay` · **Consumed by:** Screen UI pass

```cpp
world.SetUiFont(font);
auto* hud = hudGo->AddComponent<TextOverlayComponent>();
hud->SetText("HP: 100");
hud->SetScreenPosition({24.0F, 24.0F});
hud->SetFontSizePixels(18.0F);
```

### `BlendModeComponent` / `RenderLayerComponent` / `SortingGroupComponent`

```cpp
// Per-sprite blend (sprite or tilemap sibling required)
go->AddComponent<BlendModeComponent>(SceneBlendMode::Additive);

// Named layer from RenderLayerRegistry
go->AddComponent<RenderLayerComponent>("Characters", 10);

// Batch sort for subtree (descendant sprites/tilemaps)
parent->AddComponent<SortingGroupComponent>(100);
```

---

## Lighting

### `DirectionalLightComponent`

**Kind:** `DirectionalLight` · **Consumed by:** `FillStandardLitSceneFromWorld`

Light direction = owner transform **local +Z** in world space.

```cpp
auto* sun = sunGo->AddComponent<DirectionalLightComponent>(Vector3{1,0.97,0.9}, 1.2F);
sunGo->AddComponent<TransformComponent>()->SetRotation(
    Quaternion::FromAxisAngle(Vector3::UnitX, -0.6F));
```

### `PointLightComponent` / `SpotLightComponent`

```cpp
go->AddComponent<PointLightComponent>(Vector3::One, 4.0F, 12.0F);

auto* spot = go->AddComponent<SpotLightComponent>(Vector3::One, 6.0F, 15.0F, 25.0F, 40.0F);
// Cone axis = local −Z in world space
```

---

## Camera

### `CameraComponent` (3D)

**Kind:** `Camera` · **Consumed by:** `TryResolveMainCamera`, scene submit

```cpp
auto* cam = cameraGo->AddComponent<CameraComponent>();
cam->SetProjectionMode(CameraProjectionMode::Perspective);
cam->SetFovYDegrees(72.0F);
cam->SetNearPlane(0.1F);
cam->SetFarPlane(500.0F);
cam->SetPriority(10);  // higher wins as "main" camera
```

### `Camera2DComponent` + `Camera2DRigComponent`

```cpp
auto* cam2d = cameraGo->AddComponent<Camera2DComponent>();
cam2d->SetHalfExtentY(6.0F);

auto* rig = cameraGo->AddComponent<Camera2DRigComponent>();
rig->SetMode(Camera2DRigMode::FollowTarget);
rig->SetTarget(player);
rig->SetFollowSmoothRate(7.5F);
rig->SetLookAheadScale(0.15F);
```

### `SpringArm3DComponent` + `CameraFollow3DComponent`

Third-person orbit (runs at priorities 295 / 300).

```cpp
GameObject* rig = world.CreateGameObject();
GameObject* camObj = world.CreateGameObject();
world.SetParent(camObj, rig);

rig->AddComponent<TransformComponent>();
auto* arm = camObj->AddComponent<SpringArm3DComponent>();
arm->SetPivotTarget(player);
arm->SetSocketOffset({0.0F, 1.5F, 0.0F});
arm->SetArmLength(4.5F);
arm->SetYawRadians(0.0F);
arm->SetPitchRadians(-0.3F);

camObj->AddComponent<CameraComponent>();
auto* follow = camObj->AddComponent<CameraFollow3DComponent>();
follow->SetTarget(player);
follow->SetTargetOffset({0.0F, 1.6F, 0.0F});
follow->SetLookAtTarget(true);
```

---

## Physics 2D

Call each frame from `OnUpdate` (after gameplay input, before render):

```cpp
#include "spark/physics/PhysicsSubsystem.hpp"

PhysicsSubsystem physics;  // member on your Game class
physics.Simulate2D(world, timing);
```

### `Rigidbody2DComponent`

```cpp
auto* rb = go->AddComponent<Rigidbody2DComponent>(RigidbodyBodyType2D::Dynamic, 1.0F);
rb->SetVelocity({3.0F, 0.0F});
```

### `BoxCollider2DComponent` / `CircleCollider2DComponent`

```cpp
auto* box = go->AddComponent<BoxCollider2DComponent>(Vector2{0.5F, 0.5F});
box->SetCategoryBits(1u << 0);
box->SetMaskBits(0xFFFF);
box->SetIsTrigger(false);

// On same dynamic body, circle wins over box if both exist:
go->AddComponent<CircleCollider2DComponent>(0.45F);
```

### `PolygonCollider2DComponent`

**Static only** (no dynamic rigidbody, or static/kinematic body). Convex, max **16** vertices.

```cpp
Array<Vector2> verts;
verts.PushBack({-1.0F, 0.0F});
verts.PushBack({1.0F, 0.0F});
verts.PushBack({0.0F, 1.5F});
auto* poly = ramp->AddComponent<PolygonCollider2DComponent>();
poly->SetVertices(verts);
poly->SetCategoryBits(1u << 1);
```

### `PhysicsMaterial2DComponent`

**Kind:** `PhysicsMaterial2D` · **Sibling:** static 2D collider on same object · **Consumed by:** static broad-phase bake + contact restitution

```cpp
ice->AddComponent<BoxCollider2DComponent>(Vector2{4.0F, 0.1F});
ice->AddComponent<PhysicsMaterial2DComponent>(0.15F, 0.35F);  // friction, restitution
```

### `DistanceJoint2DComponent` / `HingeJoint2DComponent`

**Consumed by:** `PhysicsWorld2D` when `PhysicsWorld2DSettings::jointIterations > 0` (default **4**)

```cpp
PhysicsWorld2DSettings settings{};
settings.jointIterations = 6;

chainA->AddComponent<Rigidbody2DComponent>();
chainB->AddComponent<Rigidbody2DComponent>();
chainA->AddComponent<DistanceJoint2DComponent>(chainB, 1.2F);

// Pin two bodies at local anchor offsets:
hingeA->AddComponent<HingeJoint2DComponent>(hingeB);
auto* hj = hingeA->GetComponent<HingeJoint2DComponent>();
hj->SetLocalAnchorA({0.0F, 0.5F});
hj->SetLocalAnchorB({0.0F, -0.5F});
```

### Layer filter

```
collide = (maskA & categoryB) && (maskB & categoryA)
```

---

## Physics 3D

```cpp
physics.SimulateAll3D(world, timing);
// or step individually:
physics.Simulate3D(world, timing);
physics.SimulateCharacterControllers3D(world, timing);
physics.SimulateTriggerVolumes3D(world, timing);
```

### `Rigidbody3DComponent` + `SphereCollider3DComponent`

```cpp
go->AddComponent<Rigidbody3DComponent>(RigidbodyBodyType3D::Dynamic, 1.0F);
go->AddComponent<SphereCollider3DComponent>(0.5F);
go->AddComponent<PhysicsMaterial3DComponent>(0.5F, 0.4F, 0.3F);
```

### `CapsuleCollider3DComponent`

Static when body is not dynamic; dynamic with `Rigidbody3DComponent`.

```cpp
go->AddComponent<CapsuleCollider3DComponent>(0.35F, 1.8F, CapsuleCollider3DComponent::Axis::Y);
```

### `MeshCollider3DComponent`

**Kind:** `MeshCollider3D` · **Sibling:** `MeshComponent` · **Static only** (non-dynamic rigidbody)

World AABB baked from mesh vertices each broad-phase rebuild — useful for imported glTF level geometry.

```cpp
level->AddComponent<MeshComponent>(levelMesh, SceneMeshSlot::Custom, Vector3::One);
level->AddComponent<MeshCollider3DComponent>();
```

### `CharacterController3DComponent`

Kinematic motor — **do not** add `Rigidbody3DComponent` on the same object.

```cpp
auto* cc = player->AddComponent<CharacterController3DComponent>(0.4F, Vector3{0,0.9F,0});
cc->SetMoveInput({inputX, 0.0F, inputZ});
// After SimulateCharacterControllers3D:
const bool grounded = cc->IsGrounded();
```

### `TriggerVolume3DComponent`

```cpp
auto* trig = zone->AddComponent<TriggerVolume3DComponent>(
    TriggerVolume3DShape::Box, Vector3{2,2,2});
trig->SetOnEnter([](GameObject& other) { (void)other; });
trig->SetOnExit([](GameObject& other) { (void)other; });
```

### `DistanceJoint3DComponent`

Requires `jointIterations > 0` in `PhysicsWorld3DSettings`. Both ends need `SphereCollider3DComponent`.

```cpp
PhysicsWorld3DSettings settings{};
settings.jointIterations = 8;

ballA->AddComponent<DistanceJoint3DComponent>(ballB, 2.5F);
auto* joint = ballA->GetComponent<DistanceJoint3DComponent>();
joint->SetStiffness(0.9F);
```

### `HingeJoint3DComponent` / `SpringJoint3DComponent`

**Hinge:** soft pin at local anchor offsets (revolute-style position lock).  
**Spring:** velocity spring-damper along the line between sphere centers.

```cpp
doorA->AddComponent<HingeJoint3DComponent>(doorB);
auto* hinge = doorA->GetComponent<HingeJoint3DComponent>();
hinge->SetLocalAnchorA({0.0F, 1.0F, 0.0F});
hinge->SetLocalAnchorB({0.0F, 1.0F, 0.0F});

pendulum->AddComponent<SpringJoint3DComponent>(anchor, 3.0F);
auto* spring = pendulum->GetComponent<SpringJoint3DComponent>();
spring->SetSpringStiffness(55.0F);
spring->SetDamping(5.0F);
```

### `CollisionComponent` (legacy sphere)

Simple world-space sphere bounds; used by some probe paths.

```cpp
go->AddComponent<CollisionComponent>(0.5F, Vector3::Zero);
```

---

## Animation

### `Character3DAnimFsmComponent`

**Priority:** 100 · Add **before** `AnimatorComponent` on same object.

```cpp
go->AddComponent<Character3DAnimFsmComponent>();
go->AddComponent<AnimatorComponent>(skeleton, walkClip, 1.0F);
auto* fsm = go->GetComponent<Character3DAnimFsmComponent>();
fsm->SetLocomotionInput({0.0F, 0.0F, 1.0F});
fsm->RequestAttack();
```

### `AnimationEventReceiverComponent`

**Priority:** 210 · **Sibling:** `AnimatorComponent`

```cpp
auto* recv = go->AddComponent<AnimationEventReceiverComponent>();
recv->AddMarker(0, 0.35F, "Footstep");
recv->AddMarker(0, 0.72F, "Footstep");

class FootstepHandler final : public GameComponent {
public:
    void OnSignal(GameObject& owner, SignalId id, const SignalPayload& p) override {
        if (id == SignalId::AnimationEvent) {
            // const char* name = static_cast<const char*>(p.ptr);
        }
    }
};
go->AddComponent<FootstepHandler>();
```

### `AttachmentSocketComponent`

**Priority:** 250 · Source needs `AnimatorComponent`.

```cpp
// On character:
go->AddComponent<AnimatorComponent>(skeleton, 0);

// On weapon child (or separate object):
auto* socket = go->AddComponent<AttachmentSocketComponent>();
socket->SetSourceObject(character);
socket->SetAttachedObject(swordObject);
socket->SetJointIndex(12);  // hand bone
socket->SetLocalOffset({0.05F, 0.0F, 0.1F});
```

### `Sprite2DCharacterAnimFsmComponent`

**Priority:** 100 · **Sibling:** `SpriteAnimatorComponent`

```cpp
go->AddComponent<Sprite2DCharacterAnimFsmComponent>();
go->AddComponent<SpriteAnimatorComponent>();
auto* fsm = go->GetComponent<Sprite2DCharacterAnimFsmComponent>();
fsm->RequestAttack();
fsm->RequestHurt();
```

---

## AI

`SimulateGameAi` runs, in order: `ProcessNavMeshAgents` → `ProcessPerceptionSensors` → each enabled `AiAgentComponent::SubsystemTick`.

### `AiAgentComponent`

**Consumed by:** `SimulateGameAi` (not `OnUpdate` on the component)

```cpp
auto* agent = enemy->AddComponent<AiAgentComponent>();
AiBlackboard& bb = agent->GetBlackboard();
bb.SetFloat(0, playerX);   // grid goal X when using NavMeshAgent grid mode
bb.SetFloat(1, playerZ);
agent->SetMaxSpeed(4.5F);
agent->SetSteeringPlane(AiSteeringPlane::XzWorld);
agent->SetFsmEnabled(true);
```

### `PatrolPathComponent` + `NavMeshAgentComponent`

**PatrolPath** stores local waypoints; **NavMeshAgent** copies them into `AiAgentComponent::pathWorldPolylineXZ` each AI tick (or runs grid A* when `UseGridPathfinding()` is true).

```cpp
// Waypoint object (or same object as agent):
auto* path = pathObj->AddComponent<PatrolPathComponent>();
path->GetWaypoints().PushBack({0.0F, 0.0F, 0.0F});
path->GetWaypoints().PushBack({8.0F, 0.0F, 0.0F});
path->GetWaypoints().PushBack({8.0F, 0.0F, 8.0F});
path->SetLooping(true);

enemy->AddComponent<AiAgentComponent>();
auto* nav = enemy->AddComponent<NavMeshAgentComponent>();
nav->SetPatrolPathObject(pathObj);

// Optional grid replan to blackboard goal (slots 0,1 = world X/Z):
nav->SetUseGridPathfinding(true);
nav->SetGridOriginXZ({-32.0F, -32.0F});
nav->SetGridCellSize(1.0F);
```

### `PerceptionSensorComponent`

**Consumed by:** `ProcessPerceptionSensors` — fills `GetDetectedObjects()` each frame (sight cone + hearing radius).

```cpp
auto* sense = guard->AddComponent<PerceptionSensorComponent>();
sense->SetSightRadius(18.0F);
sense->SetSightFovDegrees(110.0F);
sense->SetHearingRadius(10.0F);

// After SimulateGameAi:
for (GameObject* target : sense->GetDetectedObjects()) {
    (void)target;
}
```

---

## Audio

`Game::OnUpdate` calls `ProcessSoundCues`, which runs `ProcessAudioListeners`, then `ProcessAmbientZones`, then flushes cues.

### `AudioListenerComponent`

```cpp
cameraGo->AddComponent<AudioListenerComponent>()->SetPriority(10);
```

Without an explicit listener, spatial audio falls back to the main 3D camera pose.

### `SoundCueComponent`

```cpp
auto* cue = player->AddComponent<SoundCueComponent>();
cue->Queue(jumpClip, 0.9F);

// Spatial one-shot at world position:
const Matrix4& wm = player->GetWorldMatrix();
const Vector3 pos{wm.m[12], wm.m[13], wm.m[14]};
cue->QueueAtWorld(landClip, 0.7F, pos, 1.0F, 1.0F, 48.0F);
```

Background music: `context.TryGetSoundEngine()->SetBackgroundMusic(bgm, 0.3F, true)`.

### `AmbientZoneComponent`

**Consumed by:** `ProcessAmbientZones` — when the listener is inside, scales master one-shot volume (`volumeScale`). Higher `priority` wins.

```cpp
auto* zone = cave->AddComponent<AmbientZoneComponent>();
zone->SetShape(VolumeShape::Sphere);
zone->SetHalfExtents({10.0F, 10.0F, 10.0F});
zone->SetVolumeScale(0.7F);
zone->SetPriority(3);
```

---

## UI

### `UiCanvasComponent`

```cpp
#include "spark/ui/Ui.hpp"

auto canvas = uiRoot->AddComponent<UiCanvasComponent>();
canvas->SetSortOrder(0);
canvas->SetTheme(Ui::UiTheme::ClassicMint());

Ui::IUiControlsFactory& factory =
    Ui::UiSystem::Get().GetActiveBackendPtr()->GetControlsFactory();
Ui::PanelDesc panelDesc{};
panelDesc.id = Utf8String("hud");
auto panel = factory.CreatePanel(panelDesc);
canvas->SetRoot(MoveTemp(panel));

// Each frame:
ProcessUiCanvasesInput(world, input, fbW, fbH);
// After FillStandardLitSceneFromWorld (or equivalent):
PaintUiCanvases(world, params, fbW, fbH);
```

---

## World

### `SceneSpatialPolicyComponent`

```cpp
go->AddComponent<SceneSpatialPolicyComponent>(ScenePartitionKind::BoundingVolumeHierarchy);
```

Affects `Scene::ForEachDrawableInViewFrustum` partition strategy.

### `TimeOfDayDriverComponent`

**Consumed by:** `ProcessTimeOfDayDrivers` before scene lighting resolve

Drives `SceneRenderParams::useTimeOfDay` and `timeOfDay` (0 = midnight, 0.5 = noon). When `loop` and `dayLengthSeconds > 0`, time advances automatically each submit.

```cpp
auto* tod = worldRoot->AddComponent<TimeOfDayDriverComponent>();
tod->SetTimeOfDay(0.35F);
tod->SetDayLengthSeconds(90.0F);
tod->SetLooping(true);
tod->SetPriority(1);
```

---

## Gameplay

### `HealthComponent` + `DamageableComponent`

```cpp
auto* hp = enemy->AddComponent<HealthComponent>(100.0F);
auto* dmg = enemy->AddComponent<DamageableComponent>();
dmg->SetDamageMultiplier(1.5F);  // vulnerable to fire, etc.

float applied = dmg->ApplyDamage(25.0F, player);
hp->SetOnDeath([](GameObject& self, GameObject* killer) {
    (void)killer;
    self.GetWorld().DestroyGameObject(&self);
});
```

Listen for combat feedback:

```cpp
void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override {
    if (id == SignalId::DamageApplied) {
        const auto* d = static_cast<const DamageSignalPayload*>(payload.ptr);
        // d->applied, d->instigator, d->target
    }
}
```

---

## Typical object recipes

### 2D platformer player

```cpp
player->AddComponent<TransformComponent>();
player->AddComponent<SpriteComponent>(heroTex);
player->AddComponent<SpriteAnimatorComponent>();
player->AddComponent<Sprite2DCharacterAnimFsmComponent>();
player->AddComponent<Rigidbody2DComponent>();
player->AddComponent<BoxCollider2DComponent>();
player->AddComponent<SoundCueComponent>();
player->AddComponent<HealthComponent>(100.0F);
player->AddComponent<DamageableComponent>();
```

### 3D skinned enemy

```cpp
enemy->AddComponent<TransformComponent>();
enemy->AddComponent<SkinnedMeshComponent>(mesh);
enemy->AddComponent<MaterialComponent>(tex);
enemy->AddComponent<Character3DAnimFsmComponent>();
enemy->AddComponent<AnimatorComponent>(skeleton, 0);
enemy->AddComponent<CapsuleCollider3DComponent>(0.4F, 1.8F);
enemy->AddComponent<Rigidbody3DComponent>();
enemy->AddComponent<HealthComponent>(50.0F);
enemy->AddComponent<DamageableComponent>();
```

### 3D patrolling guard

```cpp
enemy->AddComponent<TransformComponent>();
enemy->AddComponent<SkinnedMeshComponent>(mesh);
enemy->AddComponent<AiAgentComponent>();
enemy->AddComponent<NavMeshAgentComponent>()->SetPatrolPathObject(patrolPathObj);
enemy->AddComponent<PerceptionSensorComponent>();
enemy->AddComponent<HealthComponent>(80.0F);
```

### Static 2D level chunk

```cpp
level->AddComponent<TransformComponent>();
level->AddComponent<TilemapComponent>(...);
level->AddComponent<TilemapCollider2DComponent>();
```

---

## Serialization

Scene format **`spark_scene_v4`** captures a subset of components via `ComponentSnapshotRegistry`. Registered kinds include Transform, Mesh, Material, lights, cameras, 3D physics, Health, PolygonCollider2D, and more — see `SceneSerializer.cpp` `kCaptureOrder`.

Components without handlers still work at runtime; they are omitted from saved scenes until a handler is added.

---

## Related chapters

- [ECS and Scene](06-ecs-and-scene.md) — `GameObject`, signals, submit hand-off
- [Colliders](../5-physics/03-colliders.md) — layer masks, triggers
- [Sound Cues](../6-sound/03-cues.md) — mixer and spatial audio
- [Cameras in 3D](../3-3d-graphics/02-cameras-3d.md) — fly camera vs ECS rigs
- [`ARCHITECTURE_AND_DEVELOPER_GUIDE.md`](../../ARCHITECTURE_AND_DEVELOPER_GUIDE.md) — contributor deep-dive
