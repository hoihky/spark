# Camera2D

The 2D camera stack has two layers:

1. **`Camera2D` struct** — pure math: orthographic view/projection and billboard basis for particles.
2. **ECS rig** — `Camera2DComponent` + `Camera2DRigComponent` + `ScreenShakeComponent` + optional `ParallaxLayerComponent` on backgrounds.

Production platformers should use the ECS rig so follow smoothing, bounds, shake, and parallax share one update order.

## Class design: `Camera2D` (math)

`spark/scene/camera/Camera2D.hpp`

| Field | Default | Meaning |
|-------|---------|---------|
| `position` | `(0,0,0)` | Frustum center in world XY |
| `rotationRad` | `0` | Roll around Z |
| `halfExtentY` | `5` | Half-height of ortho view (world units) |
| `clipNearZ` / `clipFarZ` | `-500` / `500` | Depth clip range |

```cpp
struct Camera2D {
    Matrix4 ViewMatrix() const noexcept;
    Matrix4 ViewProjection(float framebufferWidth, float framebufferHeight) const noexcept;
    void BillboardBasisWorld(Vector3& outRight, Vector3& outUp) const noexcept;
};
```

`ViewProjection` uses `OrthographicVulkan` — framebuffer **Y increases downward** in NDC. Picking and HUD placement must use the same convention as `TerrainScreenToWorldRay` (see [Tilemaps — screen picking](03-tilemaps.md#screen--cell-picking-2d)).

**Pixel-perfect sizing:** choose `halfExtentY` so `fbHeight / (2 * halfExtentY)` ≈ your desired pixels-per-world-unit. Lock gameplay positions to multiples of `1/PPU` when aligning to tile grids.

## Class design: `Camera2DComponent`

Stores the same parameters on a `GameObject`. `FillStandardLitSceneFromWorld` can resolve the active camera from tagged components, or your game can copy `Camera2DComponent` fields into a local `Camera2D` each frame (Platformer2D pattern).

## Concept: submit the scene with a 2D camera

### C++

```cpp
int fbW = 0, fbH = 0;
context.GetFramebufferSize(fbW, fbH);
if (fbW <= 0) fbW = 1;
if (fbH <= 0) fbH = 1;

Camera2D camera{};
camera.position = {playerX, playerY, 0.0F};
camera.halfExtentY = 6.5F;

const Matrix4 viewProj = camera.ViewProjection(static_cast<float>(fbW), static_cast<float>(fbH));
Vector3 particleRight{}, particleUp{};
camera.BillboardBasisWorld(particleRight, particleUp);

SubmitStandardLitSceneFromWorld(
    GetWorld(), context, viewProj, camera.position,
    Vector3{0.30F, 0.86F, 0.36F}.Normalized(),
    Vector3{1.0F, 0.98F, 0.95F}, 0.85F,
    Vector3{0.16F, 0.18F, 0.24F},
    false, particleRight, particleUp,
    sceneTimeSeconds,
    SceneSpriteSortMode::SortOrderThenWorldY);
```

### C\#

```csharp
int fbW = 0, fbH = 0;
context.GetFramebufferSize(ref fbW, ref fbH);
fbW = Math.Max(fbW, 1);
fbH = Math.Max(fbH, 1);

var cam2d = cameraGo.GetComponent<Camera2DComponent>();
var position = cam2d.GetWorldPosition();
var viewProj = cam2d.ComputeViewProjection(fbW, fbH);

SceneSubmit.SubmitStandardLitSceneFromWorld(
    world, context, viewProj, position,
    sunDir, sunColor, sunIntensity, ambient,
    enableParticles: false,
  particleRight: default, particleUp: default,
    sceneTimeSeconds,
    SceneSpriteSortMode.SortOrderThenWorldY);
```

(Exact helper names follow your game's `SceneSubmit` wrapper in C++ samples; C# games often mirror `OnRender` from `Platformer2DGame` in native template code.)

### Python

```python
camera = world.create_game_object("MainCamera")
camera.get_or_add_transform()
camera.add_camera_2d(half_extent_y=4.0, priority=10)
```

The script host drives render submit; you configure **`Camera2DComponent`** on the object. Use `Camera2DRigComponent` bindings to set follow target from Python when exposed in your game's script layer.

## Concept: follow rig (recommended)

Update priorities (same object tree):

| Component | Priority | Role |
|-----------|----------|------|
| `ParallaxLayerComponent` | 290 | Background scroll factor |
| `ScreenShakeComponent` | 295 | Offset added to camera |
| `Camera2DRigComponent` | 300 | Follow / bounds / look-ahead |
| `Camera2DComponent` | — | Half extent + view source |

### C++

```cpp
GameObject* cameraGo = world.CreateGameObject();
cameraGo->AddComponent<Camera2DComponent>()->SetHalfExtentY(6.5F);
Camera2DRigComponent* rig = cameraGo->AddComponent<Camera2DRigComponent>();
rig->SetMode(Camera2DRigMode::FollowTarget);
rig->SetTarget(player);
rig->SetFollowSmoothRate(8.0F);
rig->SetLookAheadScale(0.12F);
cameraGo->AddComponent<ScreenShakeComponent>();
```

### C\#

```csharp
var cameraGo = world.CreateGameObject("Camera");
cameraGo.AddCamera2D(6.5f, priority: 0);
var rig = cameraGo.AddCamera2DRig();
rig.SetMode(Camera2DRigMode.FollowTarget);
rig.SetTarget(player);
rig.SetFollowSmoothRate(8f);
rig.SetLookAheadScale(0.12f);
cameraGo.AddScreenShake();
```

### Python

```python
cam = world.create_game_object("Camera")
cam.add_camera_2d(6.5, priority=0)
rig = cam.add_camera_2d_rig()
rig.set_mode_follow_target()
rig.set_target(player)
rig.set_follow_smooth_rate(8.0)
cam.add_screen_shake()
```

## Concept: parallax backgrounds

Distant sprites move slower than the camera. Reference the camera `GameObject` on each layer.

### C++

```cpp
bgGo->AddComponent<SpriteComponent>(skyTex, tint, uv, /*sortOrder=*/5);
ParallaxLayerComponent* parallax = bgGo->AddComponent<ParallaxLayerComponent>();
parallax->SetFactorX(0.06F);
parallax->SetFactorY(0.0F);
parallax->SetCameraReference(cameraGo);
```

### C\#

```csharp
sky.AddSprite("bg/sky", tint, uv, sortOrder: 5);
var parallax = sky.AddParallaxLayer();
parallax.SetFactorX(0.06f);
parallax.SetCameraReference(cameraGo);
```

### Python

```python
sky = world.create_game_object("Sky")
sky.add_sprite("bg/sky", tint, uv, sort_order=5)
pl = sky.add_parallax_layer()
pl.set_factor_x(0.06)
pl.set_camera_reference(cam)
```

See `Platformer2DDemo::SpawnBackgroundLayers` for stacked sky, mountains, hills, and drifting clouds.

## Concept: screen shake

`ScreenShakeComponent` applies a decaying offset each frame; the rig reads it when positioning the camera.

### C++

```cpp
if (ScreenShakeComponent* shake = cameraGo->GetComponent<ScreenShakeComponent>()) {
    shake->AddTrauma(0.35F);  // on heavy landing
}
```

### C\#

```csharp
cameraGo.GetComponent<ScreenShakeComponent>()?.AddTrauma(0.35f);
```

### Python

```python
cam.get_component_screen_shake().add_trauma(0.35)
```

## Legacy manual lerp

Only when **not** using `Camera2DRigComponent`:

```cpp
const Vector3 p = playerTr->GetLocalTransform().translation;
const float follow = std::min(1.0F, 8.0F * timing.deltaTimeSeconds);
camera.position.x += (p.x - camera.position.x) * follow;
camera.position.y += ((p.y + 0.85F) - camera.position.y) * follow;
```

## Sorting reminder

Separate **world** layers via `sortOrder` on sprites/tilemaps. HUD belongs in `TextOverlayComponent`, GUI canvases, or screen-space overlays — not pulled toward the camera in world space unless intentional.

Next: [Tilemaps](03-tilemaps.md).
