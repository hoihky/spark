# Character and third-person camera

`CharacterCameraDemo` (**#5**, recommended) is the reference for **skinned glTF**, **first/third person toggle**, `CharacterController3DComponent`, and `SpringArm3DComponent` collision-aware camera.

**Part 7:** [Camera](../7-game-components-usage/05-camera.md) · [Animation](../7-game-components-usage/08-animation.md) · [Assets](../1-overview-architecture/12-assets-and-content-pipeline.md).

## What the demo proves

| System | APIs |
|--------|------|
| Asset load | `RequestGltf` + `Assets().Pump()` (C++) / `Assets.Pump` (C#) + `LoadSkinnedGltf` |
| Locomotion | `CharacterController3DComponent`, `SimulateCharacterControllers3D` |
| Camera | `SpringArm3DComponent`, `CameraFollow3DComponent`, cursor capture |
| Animation | `AnimatorComponent`, `Character3DAnimFsmComponent` |

## Concept: load skinned character

### C++

```cpp
world.Assets().RequestGltf("assets/models/Fox.glb");

void OnUpdate(const FrameTiming& timing, IEngineContext& ctx) override {
    world.Assets().Pump();
    if (!foxReady && world.Assets().IsGltfReady("assets/models/Fox.glb")) {
        SkinnedGltfAsset fox = world.Assets().LoadSkinnedGltf("assets/models/Fox.glb");
        player->AddComponent<SkinnedMeshComponent>(fox.mesh);
        player->AddComponent<AnimatorComponent>(fox.skeleton, 0, 1.0F);
        foxReady = true;
    }
    physics.GetCharacterController3D().SimulateCharacterControllers3D(GetWorld(), timing);
    Game::OnUpdate(timing, ctx);
    physics.SimulateAll3D(GetWorld(), timing);
}
```

### C\#

```csharp
const string foxPath = "assets/models/Fox.glb";
World.Assets.Pump();
while (!World.Assets.IsGltfReady(foxPath))
    World.Assets.Pump();
var fox = World.Assets.LoadSkinnedGltf(foxPath);
player.AddSkinnedMesh(fox.Mesh);
player.AddAnimator(fox.Skeleton, 0, 1f);
```

### Python

```python
# glTF load path C++/C#-first — host CharacterCameraDemo or bind world.Assets().Pump() in py.
```

## Concept: character controller

### C++

```cpp
auto* cc = player->AddComponent<CharacterController3DComponent>();
cc->SetMaxWalkSpeed(6.0F);
cc->SetJumpSpeed(5.5F);

// After reading input:
cc->SetMoveIntentWorld(moveDirXZ);
cc->SetJumpRequested(wantJump);
```

Run **`SimulateCharacterControllers3D` before or with** `SimulateAll3D` per showcase ordering ([Physics 3D path](08-physics-3d-product-path.md)).

### C\#

```csharp
player.AddCharacterController3D().SetMaxWalkSpeed(6f);
```

### Python

```python
player.get_or_add_default_component(ComponentKind.CharacterController3D)
```

## Concept: spring arm + follow camera

### C++

```cpp
cameraGo->AddComponent<CameraComponent>();
auto* arm = cameraGo->AddComponent<SpringArm3DComponent>();
arm->SetPivotTarget(player);
arm->SetLength(3.5F);
arm->SetCollisionProbeRadius(0.25F);

auto* follow = cameraGo->AddComponent<CameraFollow3DComponent>();
follow->SetShoulderOffset({0.4F, 1.6F, 0.0F});
```

### C\#

```csharp
camGo.AddSpringArm3D().SetPivotTarget(player).SetLength(3.5f);
camGo.AddCameraFollow3D().SetShoulderOffset(new Vector3(0.4f, 1.6f, 0f));
```

### Python

```python
from spark._generated.component_kind import ComponentKind
cam.get_or_add_default_component(ComponentKind.SpringArm3D)
```

## Input: first vs third person

Demo toggles camera distance and cursor capture — mirror `IInput::SetCursorCaptured` when entering aim mode vs menu ([FPS shooting](04-shooting.md) uses the same pattern).

## Frame order (recommended)

1. `world.UpdateGameObjects` — input, animators  
2. Set CC3D intent from `PlayerInput` or raw axes  
3. `SimulateCharacterControllers3D`  
4. `SimulateAll3D`  
5. `SimulateGameAi` if NPCs present  
6. `OnRender` — `SubmitStandardLitSceneFromWorld` or `WithCamera`

Contrast with **Part 9 FPS** chapters: fly camera without CC3D until you adopt ch. **07**.

Next: [Physics 3D product path](08-physics-3d-product-path.md).
