# Camera2D

## Class Design: `Camera2D`

`Camera2D` (`spark/scene/camera/Camera2D.hpp`) is a **plain struct** (not a component) holding orthographic view parameters:

| Field | Default | Meaning |
|-------|---------|---------|
| `position` | `(0,0,0)` | Camera center in world XY |
| `rotationRad` | `0` | Roll around Z |
| `halfExtentY` | `5` | Half-height of ortho frustum |
| `clipNearZ` / `clipFarZ` | `-500` / `500` | Depth range |

```cpp
struct Camera2D {
    Vector3 position{0.0F, 0.0F, 0.0F};
    float rotationRad = 0.0F;
    float halfExtentY = 5.0F;

    Matrix4 ViewMatrix() const noexcept;
    Matrix4 ViewProjection(float framebufferWidth, float framebufferHeight) const noexcept;
    void BillboardBasisWorld(Vector3& outRight, Vector3& outUp) const noexcept;
};
```

## Apply to Render Submit

```cpp
int fbW = 0, fbH = 0;
context.GetFramebufferSize(fbW, fbH);
const Matrix4 viewProj = camera.ViewProjection(static_cast<float>(fbW), static_cast<float>(fbH));

Vector3 particleRight{}, particleUp{};
camera.BillboardBasisWorld(particleRight, particleUp);

SubmitStandardLitSceneFromWorld(
    GetWorld(), context, viewProj, camera.position,
    Vector3{0.3F, 0.86F, 0.36F}.Normalized(),  // sun direction
    Vector3{1.0F, 0.98F, 0.95F}, 0.85F,       // sun color + intensity
    Vector3{0.16F, 0.18F, 0.24F},             // ambient
    false,                                     // enableParticles
    particleRight, particleUp,
    sceneTimeSeconds,
    SceneSpriteSortMode::SortOrderThenWorldY);
```

## ECS Camera Rig (recommended)

For gameplay projects, prefer `Camera2DRigComponent` over manual lerping. It handles follow smoothing, look-ahead, bounds clamping, and reads `ScreenShakeComponent` offset on the same object.

```cpp
#include "spark/ecs/components/camera/Camera2DComponent.hpp"
#include "spark/ecs/components/camera/Camera2DRigComponent.hpp"
#include "spark/ecs/components/camera/ScreenShakeComponent.hpp"

GameObject* cameraGo = world.CreateGameObject();
cameraGo->AddComponent<Camera2DComponent>()->SetHalfExtentY(6.5F);
auto* rig = cameraGo->AddComponent<Camera2DRigComponent>();
rig->SetMode(Camera2DRigMode::FollowTarget);
rig->SetTarget(player);
rig->SetFollowSmoothRate(8.0F);
rig->SetLookAheadScale(0.12F);
cameraGo->AddComponent<ScreenShakeComponent>();
```

`Camera2DRigComponent` runs at priority **300**; `ScreenShakeComponent` at **295**; background `ParallaxLayerComponent` at **290**.

## Parallax Layers

Distant sprites scroll slower than the camera:

```cpp
#include "spark/ecs/components/rendering/ParallaxLayerComponent.hpp"

bgGo->AddComponent<SpriteComponent>(skyTex, ...);
auto* parallax = bgGo->AddComponent<ParallaxLayerComponent>();
parallax->SetFactorX(0.06F);
parallax->SetCameraReference(cameraGo);
```

See `Platformer2DDemo::SpawnBackgroundLayers` for a full stack (sky, mountains, hills, clouds with sine drift).

## Manual Smooth Follow (legacy / tutorials)

```cpp
const Vector3 p = playerTr->GetLocalTransform().translation;
const float follow = std::min(1.0F, 8.0F * timing.deltaTimeSeconds);
camera.position.x += (p.x - camera.position.x) * follow;
camera.position.y += ((p.y + 0.85F) - camera.position.y) * follow;
```

Use this only when you are not using `Camera2DRigComponent`.

## Pixel-Perfect Tips

- Use integer world positions for tile-aligned art.
- Set `halfExtentY` so one world unit ≈ N screen pixels at your target resolution.
- Separate **sortOrder layers**: background `10`, gameplay `100`, VFX `200`, HUD via `TextOverlayComponent` or GUI.

## Screen → world (picking)

Framebuffer coordinates match `gl_FragCoord` (origin top-left, Y down). With `Camera2D::ViewProjection` + `OrthographicVulkan`, unproject using `TerrainScreenToWorldRay` (`spark/demo/ShellDemoSceneUtil.hpp`) — **do not** apply an extra OpenGL-style `ndcY = 1 - y` flip. See [Tilemaps](03-tilemaps.md#screen--cell-picking-2d).

Next: [Tilemaps](03-tilemaps.md).
