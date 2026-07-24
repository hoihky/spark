# Camera and HUD

## Camera Follow

```cpp
camera.position = {kPlayerSpawnX, spawnY + 1.0F, 0.0F};
camera.halfExtentY = 6.5F;

// Each frame:
const float follow = std::min(1.0F, 8.0F * timing.deltaTimeSeconds);
camera.position.x += (p.x - camera.position.x) * follow;
camera.position.y += ((p.y + 0.85F) - camera.position.y) * follow;
```

## HUD Text

```cpp
hudText->SetText(Utf8String(std::format(
    "2D platformer {} | pos ({:.1f},{:.1f}) | A/D Space | ESC quit",
    goalReached ? "— GOAL!" : "",
    p.x, p.y).c_str()));
```

## Render Submit

```cpp
const Matrix4 viewProj = camera.ViewProjection(static_cast<float>(fbW), static_cast<float>(fbH));
Vector3 pr{}, pu{};
camera.BillboardBasisWorld(pr, pu);

SubmitStandardLitSceneFromWorld(
    GetWorld(), context, viewProj, camera.position,
    Vector3{0.30F, 0.86F, 0.36F}.Normalized(),
    Vector3{1.0F, 0.98F, 0.95F}, 0.85F,
    Vector3{0.16F, 0.18F, 0.24F},
    false, pr, pu, sceneTimeSeconds,
    SceneSpriteSortMode::SortOrderThenWorldY);
```

Next: [Polish and Ship](06-polish.md).
