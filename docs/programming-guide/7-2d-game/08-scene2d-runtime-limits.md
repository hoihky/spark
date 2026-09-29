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

### Authoring (ECS)

1. Create a `RenderTexture` (`CreateMinimapRenderTexture()` uses **RGBA8** for HUD copy compatibility).
2. Create a placeholder `Texture2D` for the HUD atlas layer (`CreateMinimapHudPlaceholderTexture`).
3. `IRenderTargetService::CreateRenderTarget` / `EnsureGpuResources` allocates GPU images when the Vulkan backend is active.
4. Add `Scene2DCompositeViewComponent` with `Scene2DCompositeFeature` (`Minimap`, `FogOfWarMask`, `SceneOutline`), normalized screen rect, and world ortho capture center/extent.
5. Each frame, `FillStandardLitSceneFromWorld` → `CollectScene2DCompositeViews` appends `Scene2DCompositeViewDesc` entries to `SceneRenderParams`.

### GPU capture pipeline

| Step | System |
|------|--------|
| Main world | HDR render pass (scene color `R16G16B16A16_SFLOAT`) |
| Composite capture | `VulkanScene2DCompositeCapture` — ortho VP, resubmit tilemaps/sprites via `Vulkan2DCompositePass` |
| Render pass selection | `RenderTextureFormat::Rgba8Unorm` → **LDR offscreen render pass** (`VulkanOffscreenLdrRenderPass`); `HdrRGBA16Float` → HDR pass |
| Pipelines | Sprite pass maintains **HDR** and **LDR offscreen** pipeline variants (same shaders, compatible subpass layout) |
| HUD blit | `VulkanScreenUiPass::RecordCompositeRenderTextureBlits` — `vkCmdCopyImage` from offscreen RGBA8 into the UI sprite atlas layer |
| Player marker | `PatchScene2DMinimapHud` — screen-space dot using `ComputeMinimapOrthoBoundsSquare` / `WorldXYToMinimapSquareUv` |

**Why RGBA8 for minimap:** the UI atlas is `R8G8B8A8_UNORM`. HDR offscreen color cannot be copied with `vkCmdCopyImage` (texel size mismatch); minimap capture therefore uses LDR targets and the LDR render pass.

CPU fallback when GPU capture is off: `RebuildMinimapTextureFromGameplayGrid` fills a walkability tint texture.

**P0 wiring:** `Scene2DCompositeViewComponent` on the flow object (`P0GameFlow`) with `target` + `hudTexture`; `SyncMinimapCompositeView()` updates world capture from the player and grid each frame.

See [2D gameplay API — minimap](09-2d-gameplay-api-guide.md#10-minimap-and-composite-views).

## Parametric sprite FX

Use `SpriteLighting2DMode::HitFlash`, `Outline`, or `Dissolve` (13–15) or helpers in `spark/render/sprites2d/SpriteFx2D.hpp`. Params map to `SpriteLighting2DComponent::param0/param1` and the instance buffer consumed by `sprite.frag`.

| Mode | Helper | Notes |
|------|--------|-------|
| 13 Hit flash | `ApplyHitFlashAtSceneTime` | `param1.y` = scene time at flash start |
| 14 Outline | `ApplyOutline` | Screen-space edge from alpha neighbors |
| 15 Dissolve | `ApplyDissolveProgress` | Noise clip; `progress` 0 = visible, 1 = gone |

Rebuild shaders (`sprite.frag` / SPIR-V) after changing FX modes in development builds.
