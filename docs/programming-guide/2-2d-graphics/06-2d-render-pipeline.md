# 2D Render Pipeline

## Class Design: `SceneRenderParams` (2D subset)

| Field | Limit | Purpose |
|-------|-------|---------|
| `sprites` | 8192 | `SceneSpriteDraw` array |
| `sceneTextures` | 16 | GPU texture table |
| `screenTexts` | — | HUD strings |
| `screenRects` | — | GUI / debug rects |
| `spriteSortMode` | enum | Sort policy |

```cpp
enum class SceneSpriteSortMode : std::uint8_t {
    SortOrderOnly = 0,
    SortOrderThenWorldY = 1,
};
```

## End-to-End 2D Frame (Platformer Pattern)

```cpp
void Platformer2DGame::OnRender(IRenderFrame&, IEngineContext& context) {
    int fbW = 0, fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    if (fbW <= 0) fbW = 1;
    if (fbH <= 0) fbH = 1;

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
}
```

`FillStandardLitSceneFromWorld` walks ECS and fills sprites, tilemaps, text overlays, and optional particles.

## GUI Overlay on 2D

```cpp
SceneRenderParams params{};
FillStandardLitSceneFromWorld(..., params);
PaintGuiCanvases(GetWorld(), params, fbW, fbH);
context.SetSceneRenderParams(params);
```

## Update Order Reminder

```cpp
void OnUpdate(const FrameTiming& t, IEngineContext& ctx) override {
    // 1. Read input, set velocities
    // 2. SimulatePhysics2D(world, t, settings)  — explicit call!
    // 3. Gameplay rules (respawn, goals)
    Game::OnUpdate(t, ctx);  // component OnUpdate + sound cues
}
```

Part 2 complete → **Part 3**: [Meshes and Materials](../3-3d-graphics/01-meshes-and-materials.md).
