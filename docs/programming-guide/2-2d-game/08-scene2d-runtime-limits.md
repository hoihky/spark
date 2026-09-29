# 2D runtime limits and composite views

Spark submits 2D draws through `SceneRenderParams`. External editors should treat these as **hard per-frame budgets** unless a platform build explicitly raises them.

## Caps (`Scene2DRuntimeLimits`)

| Limit | Value | Field |
|-------|------:|-------|
| Sprites | 8192 | `SceneRenderParams::sprites` |
| Tilemap layers | 64 | `SceneRenderParams::tilemaps` |
| Tile instances | 65536 | `SceneRenderParams::tilemapTiles` |
| Particles | 8192 | `SceneRenderParams::particles` |
| UI textures | 16 | `SceneRenderParams::uiTextures` |
| Scene texture layers | 16 | GPU sprite array |
| Composite views | 4 | `SceneRenderParams::scene2DCompositeViews` |

## Minimap / fog / outline

1. Allocate a `RenderTexture` and ensure GPU resources via `IRenderTargetService::EnsureGpuResources`.
2. Add `Scene2DCompositeViewComponent` with `Scene2DCompositeFeature` (`Minimap`, `FogOfWarMask`, `SceneOutline`), screen rect, and ortho capture center/extent.
3. Each frame, `FillStandardLitSceneFromWorld` calls `CollectScene2DCompositeViews` so descriptors ride along with the main submit.

**P0 minimap:** `Scene2DCompositeViewComponent` on the flow object (feature `Minimap`, `target` + `hudTexture`) is collected each submit; `VulkanScene2DCompositeCapture` ortho-resubmits tilemaps/sprites, then blits into the HUD `uiTextures` layer. `PatchScene2DMinimapHud` draws the player dot using the same ortho bounds as capture (`ComputeMinimapOrthoBoundsSquare`). CPU walkability tint remains when GPU capture is disabled.

## Parametric sprite FX

Use `SpriteLighting2DMode::HitFlash`, `Outline`, or `Dissolve` (13–15) or helpers in `spark/render/sprites2d/SpriteFx2D.hpp`. Params map to `SpriteLighting2DComponent::param0/param1` and the instance buffer consumed by `sprite.frag`.
