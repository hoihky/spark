---
title: Camera2D
order: 2
---

# Camera2D

## Class Design: `Camera2D`

`Camera2D` (`spark/scene/Camera2D.hpp`) is a **plain struct** (not a component) holding orthographic view parameters:

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

## Smooth Follow (from Platformer Sample)

```cpp
const Vector3 p = playerTr->GetLocalTransform().translation;
const float follow = std::min(1.0F, 8.0F * timing.deltaTimeSeconds);
camera.position.x += (p.x - camera.position.x) * follow;
camera.position.y += ((p.y + 0.85F) - camera.position.y) * follow;
```

## Pixel-Perfect Tips

- Use integer world positions for tile-aligned art.
- Set `halfExtentY` so one world unit ≈ N screen pixels at your target resolution.
- Separate **sortOrder layers**: background `10`, gameplay `100`, VFX `200`, HUD via `TextOverlayComponent` or GUI.

Next: [Tilemaps](2d-graphics/03-tilemaps.html).
