# Blessed lit-render API (`world.Rendering()`)

Gameplay and tools should submit the standard lit pipeline through the **`GameWorld::Rendering()`** facet — not free functions in `SceneSubmit.hpp`, not `LitScene::Builder` (legacy fluent style).

## C++

| Goal | API |
|------|-----|
| Submit (fill + `SetSceneRenderParams`) | `world.Rendering().SubmitLit(context, viewProjection, cameraPos, opts)` |
| ECS main camera | `world.Rendering().SubmitLitWithCamera(context, opts)` |
| Fill only (tweak `SceneRenderParams` first) | `world.Rendering().FillLit(context, viewProjection, cameraPos, opts, params)` |
| Resolve camera into params | `world.Rendering().ResolveCamera(fbW, fbH, params)` |

Always bundle lighting, particles, sprite sort, and optional frustum culling in **`LitSceneSubmitOptions`** (`WithDirectionalLight`, `WithSceneForCulling(&scene)`, …).

```cpp
Spark::LitSceneSubmitOptions opts{};
opts.WithDirectionalLight(sunDir, sunColor, 1.0F)
    .WithAmbient(ambient)
    .WithSpriteSortMode(Spark::SceneSpriteSortMode::SortOrderThenWorldY);
if (Spark::Scene* scene = context.TryGetScene()) {
    opts.WithSceneForCulling(scene);
}
world.Rendering().SubmitLit(context, viewProjection, cameraPos, opts);
```

**2D camera helper:** build `viewProj` / billboard axes from `Camera2D`, then call `SubmitLit` (see `Platformer2DDemo`).

## C#

| C++ | C# |
|-----|-----|
| `SubmitLit` | `world.Rendering.SubmitLit(context, viewProjection, cameraPos, options)` |
| `SubmitLitWithCamera` | `world.Rendering.SubmitLitWithCamera(context, options)` |
| `FillLit` | `world.Rendering.FillLit(...)` (blittable params buffer) or `FillStandardLitSceneParams` until renamed in your checkout |
| 2D | `world.Rendering.SubmitLit(context, camera2D, options)` |

`LitScene.Builder` and `SceneSubmit.*` remain for compatibility; prefer **`world.Rendering`**.

## Internal / legacy (do not use in new game code)

`world.Rendering().FillLit`, `world.Rendering().SubmitLit`, `LitScene::Builder` — implementations behind `WorldRendering` / `LitScene` static methods.

See also: [SCENE_SUBMIT_AND_SPATIAL_REFACTOR.md](SCENE_SUBMIT_AND_SPATIAL_REFACTOR.md), [04-engine-loop.md](programming-guide/1-overview-architecture/04-engine-loop.md).