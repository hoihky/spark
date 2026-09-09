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
