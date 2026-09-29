#include "spark/render/sprites2d/Scene2DMinimap.hpp"

#include "spark/render/sprites2d/Scene2DRuntimeLimits.hpp"

#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/tilemap/TilemapGameplayGrid.hpp"
#include "spark/scene/texture/Texture2D.hpp"
#include "spark/scene/render/RenderTextureDesc.hpp"
#include "spark/scene/render/RenderTextureFormat.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

void RebuildMinimapTextureFromGameplayGrid(
        const TilemapGameplayGrid& grid,
        Texture2D& texture,
        const Scene2DMinimapColors& colors) noexcept {
    const std::int32_t w = grid.Width();
    const std::int32_t h = grid.Height();
    if (w <= 0 || h <= 0) {
        return;
    }
    Array<std::uint8_t> pixels{};
    pixels.Resize(static_cast<std::size_t>(w * h * 4U));
    for (std::int32_t y = 0; y < h; ++y) {
        for (std::int32_t x = 0; x < w; ++x) {
            const std::size_t i = static_cast<std::size_t>((y * w + x) * 4);
            const bool walkable = grid.IsWalkable(x, y);
            pixels[i + 0U] = walkable ? colors.walkableR : colors.blockedR;
            pixels[i + 1U] = walkable ? colors.walkableG : colors.blockedG;
            pixels[i + 2U] = walkable ? colors.walkableB : colors.blockedB;
            pixels[i + 3U] = 255U;
        }
    }
    texture.SetPixels(static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h), MoveTemp(pixels));
    texture.SetSceneUploadNearest(true);
}

void PatchScene2DMinimapHud(
        SceneRenderParams& params,
        GameWorld& world,
        const SharedPtr<Texture2D>& texture,
        const TilemapGridFrame& frame,
        const Vector2& playerWorldXY,
        const float framebufferWidth,
        const float framebufferHeight,
        const bool orthoAlignedWithGpuCapture) noexcept {
    if (!texture || frame.mapWidth == 0U || frame.mapHeight == 0U) {
        return;
    }
    if (framebufferWidth < 1.0F || framebufferHeight < 1.0F) {
        return;
    }

    static_cast<void>(world.RegisterTexture(texture, "spark/p0_demo/minimap"));

    std::int32_t layer = -1;
    for (std::size_t i = 0; i < params.uiTextures.GetSize(); ++i) {
        if (params.uiTextures[i].Get() == texture.Get()) {
            layer = static_cast<std::int32_t>(i);
            break;
        }
    }
    if (layer < 0) {
        if (params.uiTextures.GetSize() >= params.MaxUiTextures) {
            return;
        }
        params.uiTextures.PushBack(texture);
        layer = static_cast<std::int32_t>(params.uiTextures.GetSize() - 1U);
    }

    const float margin = 12.0F;
    const float size = std::min(framebufferWidth, framebufferHeight) * 0.18F;
    const float x = framebufferWidth - margin - size;
    const float y = margin;

    ScreenSpriteDraw sprite{};
    sprite.x = x;
    sprite.y = y;
    sprite.width = size;
    sprite.height = size;
    sprite.uvRect = {0.0F, 0.0F, 1.0F, 1.0F};
    sprite.textureLayer = layer;
    sprite.alpha = 1.0F;
    sprite.paintOrder = params.NextUiPaintOrder();
    params.screenOverlaySprites.PushBack(sprite);

    float u = 0.5F;
    float v = 0.5F;
    if (orthoAlignedWithGpuCapture) {
        const Scene2DMinimapOrthoBounds ortho = ComputeMinimapOrthoBoundsSquare(frame);
        WorldXYToMinimapSquareUv(playerWorldXY, ortho, u, v);
    } else {
        const Vector2 corner = frame.LocalCornerToWorldXY();
        const float mapW = static_cast<float>(frame.mapWidth) * frame.cellSize;
        const float mapH = static_cast<float>(frame.mapHeight) * frame.cellSize;
        u = mapW > 1.0e-4F ? (playerWorldXY.x - corner.x) / mapW : 0.5F;
        v = mapH > 1.0e-4F ? (playerWorldXY.y - corner.y) / mapH : 0.5F;
    }
    const float dot = std::max(4.0F, size * 0.04F);
    const float dotX = x + std::clamp(u, 0.0F, 1.0F) * size - dot * 0.5F;
    const float dotY = y + (1.0F - std::clamp(v, 0.0F, 1.0F)) * size - dot * 0.5F;

    ScreenRectDraw dotRect{};
    dotRect.x = dotX;
    dotRect.y = dotY;
    dotRect.width = dot;
    dotRect.height = dot;
    dotRect.color = {0.35F, 0.85F, 1.0F};
    dotRect.alpha = 1.0F;
    dotRect.paintOrder = params.NextUiPaintOrder();
    params.screenOverlayRects.PushBack(dotRect);

    ScreenRectDraw border{};
    border.x = x - 2.0F;
    border.y = y - 2.0F;
    border.width = size + 4.0F;
    border.height = size + 4.0F;
    border.color = {0.05F, 0.05F, 0.08F};
    border.alpha = 0.85F;
    border.paintOrder = params.NextUiPaintOrder() - 2U;
    params.screenOverlayRects.PushBack(border);
}

SharedPtr<RenderTexture> CreateMinimapRenderTexture(const std::uint32_t sizePixels) noexcept {
    RenderTextureDesc desc{};
    desc.width = sizePixels;
    desc.height = sizePixels;
    desc.colorFormat = RenderTextureFormat::Rgba8Unorm;
    desc.includeDepth = true;
    desc.name = Utf8String("MinimapRt");
    return MakeShared<RenderTexture>(MoveTemp(desc));
}

SharedPtr<Texture2D> CreateMinimapHudPlaceholderTexture(const std::uint32_t sizePixels) noexcept {
    Texture2D tex(Utf8String("MinimapHud"));
    tex = Texture2D::CreateSolid(sizePixels, sizePixels, Vector3{0.04F, 0.05F, 0.07F}, 1.0F);
    return MakeShared<Texture2D>(MoveTemp(tex));
}

void AppendGpuMinimapCompositeCapture(
        SceneRenderParams& params,
        const SharedPtr<RenderTexture>& target,
        const SharedPtr<Texture2D>& hudTexture,
        const TilemapGridFrame& frame) noexcept {
    if (!target || !hudTexture) {
        return;
    }
    if (params.scene2DCompositeViews.GetSize() >= Scene2DRuntimeLimits::MaxScene2DCompositeViews) {
        return;
    }
    Scene2DCompositeViewDesc view{};
    view.feature = Scene2DCompositeFeature::Minimap;
    view.target = target;
    view.hudTexture = hudTexture;
    FillCompositeViewFromTilemapGridFrame(view, frame);
    params.scene2DCompositeViews.PushBack(MoveTemp(view));
}

}  // namespace Spark
