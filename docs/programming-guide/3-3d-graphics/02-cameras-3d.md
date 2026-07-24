# Cameras in 3D

## ECS cameras (`CameraComponent`)

For production scenes, attach a `CameraComponent` to a `GameObject` with `TransformComponent`. Highest `priority` among enabled cameras becomes the main camera for `TryResolveMainCamera` and `SubmitStandardLitSceneFromWorld`.

```cpp
auto* cam = cameraGo->AddComponent<CameraComponent>();
cam->SetProjectionMode(CameraProjectionMode::Perspective);
cam->SetFovYDegrees(72.0F);
cam->SetNearPlane(0.1F);
cam->SetFarPlane(500.0F);
cam->SetPriority(10);
```

Orthographic mode: `SetProjectionMode(CameraProjectionMode::Orthographic)` and `SetOrthoHalfHeight`.

## Third-person rig (`SpringArm3D` + `CameraFollow3D`)

```cpp
GameObject* camObj = world.CreateGameObject();
world.SetParent(camObj, player);

auto* arm = camObj->AddComponent<SpringArm3DComponent>();
arm->SetPivotTarget(player);
arm->SetArmLength(4.5F);
arm->SetYawRadians(yaw);
arm->SetPitchRadians(-0.25F);

camObj->AddComponent<CameraComponent>();
auto* follow = camObj->AddComponent<CameraFollow3DComponent>();
follow->SetTarget(player);
follow->SetTargetOffset({0.0F, 1.6F, 0.0F});
follow->SetLookAtTarget(true);
```

Update order: `SpringArm3D` (295) places the camera along the orbit; `CameraFollow3D` (300) smooth-follows the look target.

## Class Design: `FlyCamera`

FPS-style camera struct (`spark/scene/FlyCamera.hpp`) for demos without ECS:

```cpp
struct FlyCamera {
    Vector3 position{0.0F, 4.2F, 16.0F};
    float yaw = 0.0F;
    float pitch = 0.0F;
    float moveSpeed = 5.0F;
    float mouseSensitivity = 0.12F;

    void SnapLookAt(const Vector3& target) noexcept;
    void AddLook(float deltaX, float deltaY) noexcept;
    void ProcessMovement(IInput& input, float deltaSeconds) noexcept;
    Vector3 Forward() const noexcept;
    Matrix4 ViewMatrix() const noexcept;
};
```

## Typical FPS Update Loop

```cpp
void OnAttach(IEngineContext& context) override {
    context.GetInput().SetCursorCaptured(true);
}

void OnUpdate(const FrameTiming& timing, IEngineContext& context) override {
    IInput& in = context.GetInput();
    if (in.IsCursorCaptured()) {
        camera.AddLook(in.GetMouseDeltaX(), in.GetMouseDeltaY());
        camera.ProcessMovement(in, timing.deltaTimeSeconds);
    }
    Game::OnUpdate(timing, context);
}
```

## Perspective View-Projection

```cpp
int fbW = 1, fbH = 1;
context.GetFramebufferSize(fbW, fbH);
const float aspect = static_cast<float>(fbW) / static_cast<float>(fbH);
const Matrix4 proj = Matrix4::PerspectiveVulkan(DegreesToRadians(72.0F), aspect, 0.1F, 200.0F);
const Matrix4 viewProj = proj * camera.ViewMatrix();
```

Or use `CameraComponent::ViewProjection(owner, aspect)` when using ECS cameras.

## `CharacterCameraRig`

For third-person characters without ECS rigs, see `spark/scene/CharacterCameraRig.hpp` and `CharacterCameraDemo` — orbit camera with collision pull-in.

## Billboards

`BillboardComponent` orients its owner's transform toward the main camera each frame (priority 50). Used for particles, pickups, and impostor sprites in 3D.

See [Game Component Reference](../1-overview-architecture/07-game-component-reference.md#camera).

Next: [Lighting](03-lighting.md).
