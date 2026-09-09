# Skinned Characters

## Class Design: `SkinnedGltfAsset`

```cpp
struct SkinnedGltfAsset {
    SharedPtr<SkinnedMesh> mesh;
    SharedPtr<Skeleton> skeleton;
    SharedPtr<Texture2D> baseColorTexture;
    std::uint32_t walkClipIndex = 0;
    Quaternion bindUpAlignment;
    float bindFacingYawOffset = 0.0F;
};
```

Load via `world.LoadSkinnedGltf("assets/models/Fox.glb")`.

## Sample assets (M6)

| Asset | Clips | Notes |
|-------|-------|-------|
| `assets/models/Fox.glb` | Survey, Walk, Run | Quadruped; default Character camera avatar; primary CI / IK test asset |
| `assets/models/CesiumMan.glb` | *(single unnamed walk)* | Khronos humanoid baseline |

Full clip tables, joint counts, and FSM resolve rules: [`docs/ANIMATION_SAMPLE_ASSETS.md`](../../../ANIMATION_SAMPLE_ASSETS.md).

## Components

```cpp
SkinnedGltfAsset fox = world.LoadSkinnedGltf("assets/models/Fox.glb");

auto* go = world.CreateGameObject();
go->AddComponent<TransformComponent>()->SetTranslation({0, 0, 0});

auto* skin = go->AddComponent<SkinnedMeshComponent>(fox.mesh);
auto* mat = go->AddComponent<MaterialComponent>(fox.baseColorTexture);
mat->SetRoughness(0.6F);

auto* fsm = go->AddComponent<Character3DAnimFsmComponent>();
auto* anim = go->AddComponent<AnimatorComponent>(fox.skeleton, fox.walkClipIndex);
anim->SetLoopMode(AnimLoopMode::Loop);
fsm->ConfigureLocomotionFromSkeleton(*fox.skeleton, fox.walkClipIndex);
```

`GameObject` stable-sorts components by `UpdatePriority` each frame — add the FSM and animator in any order; the FSM (priority **100**) always runs before the animator (**200**).

## Class Design: `AnimatorComponent`

- Samples animation clips into joint palette
- `UpdatePriority` = `AnimatorPlayback` (200)
- Loop modes, crossfade, dual-clip locomotion blend
- Emits skinned draws with `jointPalette` in `SceneDrawItem`

## Class Design: `Character3DAnimFsmComponent` (M2)

Higher-level **locomotion + combat** driver for `AnimatorComponent` on the same object.

| Input source | API | Behavior |
|--------------|-----|----------|
| Physics velocity | *(automatic)* | Reads `Rigidbody3DComponent` on self or parent |
| Character motor | *(automatic)* | Reads `CharacterController3DComponent` move input / velocity on self or parent |
| Gameplay gait | `SetLocomotionInput(moving, sprint)` | Discrete idle / walk / run |
| Analog speed | `SetLocomotionAnalogSpeed(m/s)` | 1D blend tree when `SetLocomotionBlendEnabled(true)` |
| AI combat | `SetCombatBlackboardIntSlot(kAiBlackboardIntCharacter3DCombatCommand)` | Blackboard ints 1–4 → hurt / attack / stagger / death |

Clip names are resolved via `ConfigureLocomotionFromSkeleton` (`idle`, `walk`, `run`, `attack`, `hurt`, … — case-insensitive substring match). See [`docs/BLENDER_GLTF_ANIMATION_EXPORT.md`](../../../BLENDER_GLTF_ANIMATION_EXPORT.md).

```cpp
fsm->SetWalkSpeedThreshold(0.35F);
fsm->SetRunSpeedThreshold(2.5F);
fsm->SetLocomotionBlendEnabled(true);
fsm->SetLocomotionAnalogSpeed(walkSpeedMetersPerSecond);
fsm->RequestAttack();
fsm->RequestHurt();
```

**Parent / child layout** (Character Camera demo): put `Rigidbody3D` or `CharacterController3D` on the **root**; put `SkinnedMesh`, `Character3DAnimFsm`, and `Animator` on a **visual child**. The FSM walks the parent chain for motor velocity.

## Performance: palette cache + submit budget (M5)

`SceneSubmit` routes skinned palette solves through `GameWorld::GetSkinnedAnimationService()`:

| Class | Role |
|-------|------|
| `SkeletonPaletteCacheKey` | Captures skeleton + quantized playback signature |
| `SkeletonPaletteCache` | Shares palette arrays for identical signatures in one frame |
| `SkinnedAnimationBudget` | Caps palette CPU solves and skinned draw submissions |
| `SkinnedPaletteResolver` | Cache → compute → bind-pose fallback strategy |
| `SkinnedAnimationService` | Per-world coordinator used by `FillStandardLitSceneFromWorld` |

Optional ECS policy (first active instance per submit):

```cpp
auto* budget = sceneRoot->AddComponent<SkinnedAnimationBudgetComponent>();
budget->SetMaxPaletteUpdatesPerFrame(48);   // 0 = unlimited
budget->SetMaxSkinnedDrawsPerFrame(96);     // 0 = unlimited
budget->SetPaletteCacheEnabled(true);
budget->SetPaletteQuantizationHz(30.0F);
```

When the palette update cap is exceeded, extra characters fall back to **bind pose** for that frame. When the skinned draw cap is exceeded, additional skinned draws are skipped.

## Skinned glTF PBR (M5-07)

Skinned imports carry the same full PBR material table as rigid glTF (`GltfMaterial` / `MaterialComponent` slots: base color, normal, metallic-roughness, emissive).

| Class | Role |
|-------|------|
| `SkinnedGltfMaterialPresenter` | Binds `SkinnedGltfAsset` materials onto `MaterialComponent` or `MultiMaterialComponent` at load time |
| `SkinnedSceneDrawMaterialApplicator` | Applies those component textures to skinned `SceneDrawItem` layers at submit time |

`GltfAssetBinder::BindSkinnedMesh` uses the presenter automatically. Custom render loops should use `SkinnedSceneDrawMaterialApplicator` (same path as `SceneSubmit`) instead of binding only the albedo texture.

Shipped skinned glTF assets (for example `Fox.glb`) carry full PBR material slots where authored in the source file.

## Foot IK + aim IK (M5-09 / M5-10)

| Class | Role |
|-------|------|
| `FootIkComponent` | Per-limb ground raycasts + two-bone foot placement |
| `AimIkComponent` | Partial spine-chain blend toward camera or target object |
| `IkGroundProbe` | Physics raycast with Y-plane fallback |
| `SkinnedIkPipeline` | Applies foot + aim solvers to a sampled pose |
| `SkinnedIkService` | Per-world IK coordinator used during palette resolve |

```cpp
auto* footIk = visual->AddComponent<FootIkComponent>();
footIk->SetLeftFootPatterns("hips", "", "leg_l");
footIk->SetRightFootPatterns("hips", "", "leg_r");
footIk->ConfigureFromSkeleton(*skeleton);
footIk->SetEnabled(true);

auto* aimIk = visual->AddComponent<AimIkComponent>();
aimIk->ConfigureFromSkeleton(*skeleton);
aimIk->SetWorldTarget(cameraPosition);
aimIk->SetWeight(0.55F);
```

`SceneSubmit` and `SkinnedAnimationService::TryResolvePalette` automatically run IK when either component is enabled on the skinned object. Character Camera demo: **K** toggles foot IK, **L** toggles aim IK.

## Attachment Points

Use `AttachmentSocketComponent` for bone-accurate weapon / VFX anchors (priority 250, after animator):

```cpp
auto* socket = characterVisual->AddComponent<AttachmentSocketComponent>();
socket->SetSourceObject(characterVisual);
socket->SetAttachedObject(swordObject);
socket->SetJointByNamePattern("hand");
socket->SetLocalOffset({0.05F, 0.0F, 0.1F});
```

## Animation Events

```cpp
auto* events = go->AddComponent<AnimationEventReceiverComponent>();
events->ImportFromSkeleton(*skeleton);  // from .spark-anim-events.json or future glTF extras

auto* melee = go->AddComponent<AnimationMeleeHitComponent>();
melee->SetFacingObject(characterRoot);
melee->AddTarget(enemy);
```

Sidecar format and export notes: [`docs/BLENDER_GLTF_ANIMATION_EXPORT.md`](../../../BLENDER_GLTF_ANIMATION_EXPORT.md).

## Root Motion

```cpp
auto* rootMotion = characterRoot->AddComponent<RootMotionComponent>();
rootMotion->SetAnimatorObject(characterVisual);
rootMotion->SetApplyTarget(characterRoot);
rootMotion->UsePatternMotionJointResolver();
rootMotion->SetInPlace(true);  // toggle false to drive transform from clip
```

See [Game Component Reference](../1-overview-architecture/07-game-component-reference.md#animation).

Next: [Terrain and Sky](06-terrain-and-sky.md).
