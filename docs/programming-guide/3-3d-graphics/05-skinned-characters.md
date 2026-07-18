---
title: Skinned Characters
order: 5
---

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

auto* anim = go->AddComponent<AnimatorComponent>(fox.skeleton, fox.walkClipIndex);
anim->SetLoopMode(AnimLoopMode::Loop);
```

## Class Design: `AnimatorComponent`

- Samples animation clips into joint palette
- `UpdatePriority` = `AnimatorPlayback` (runs after `AnimationDriver`)
- Emits skinned draws with `jointPalette` in `SceneDrawItem`

## Attachment Points

Use `AttachmentSocketComponent` for bone-accurate weapon / VFX anchors (priority 250, after animator):

```cpp
// Character
go->AddComponent<Character3DAnimFsmComponent>();
go->AddComponent<AnimatorComponent>(fox.skeleton, fox.walkClipIndex);

// Weapon (child or separate object)
auto* socket = character->AddComponent<AttachmentSocketComponent>();
socket->SetAttachedObject(swordObject);
socket->SetJointIndex(handJointIndex);
socket->SetLocalOffset({0.05F, 0.0F, 0.1F});
```

## Animation Events

```cpp
auto* events = go->AddComponent<AnimationEventReceiverComponent>();
events->AddMarker(0, 0.35F, "Footstep");
// Listen via SignalId::AnimationEvent on a sibling GameComponent
```

See [Game Component Reference](../1-overview-architecture/07-game-component-reference.html#animation).

Next: [Terrain and Sky](3d-graphics/06-terrain-and-sky.html).
