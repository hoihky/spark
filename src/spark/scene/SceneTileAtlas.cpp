#include "spark/scene/SceneTileAtlas.hpp"

namespace Spark {

namespace {

void FillUniformGridUv(
        const std::uint16_t tileId,
        const std::uint32_t atlasTilesU,
        const std::uint32_t atlasTilesV,
        Vector4& outUv) noexcept {
    outUv = {0.0F, 0.0F, 0.0F, 0.0F};
    if (atlasTilesU == 0 || atlasTilesV == 0 || tileId == TilemapComponent::kEmptyTile) {
        return;
    }
    const std::uint32_t cells = atlasTilesU * atlasTilesV;
    if (tileId >= cells) {
        return;
    }
    const std::uint32_t tx = tileId % atlasTilesU;
    const std::uint32_t ty = tileId / atlasTilesU;
    const float du = 1.0F / static_cast<float>(atlasTilesU);
    const float dv = 1.0F / static_cast<float>(atlasTilesV);
    outUv.x = static_cast<float>(tx) * du;
    outUv.y = static_cast<float>(ty) * dv;
    outUv.z = static_cast<float>(tx + 1U) * du;
    outUv.w = static_cast<float>(ty + 1U) * dv;
}

}  // namespace

void TileIdToAtlasUvRect(
        const std::uint16_t tileId,
        const std::uint32_t atlasTilesU,
        const std::uint32_t atlasTilesV,
        Vector4& outUv) noexcept {
    FillUniformGridUv(tileId, atlasTilesU, atlasTilesV, outUv);
}

void TileIdToAtlasUvRect(
        const std::uint16_t tileId,
        const std::uint32_t atlasTilesU,
        const std::uint32_t atlasTilesV,
        const float marginPixels,
        const float spacingPixels,
        const std::uint32_t textureWidth,
        const std::uint32_t textureHeight,
        const std::uint32_t tilePixelWidth,
        const std::uint32_t tilePixelHeight,
        Vector4& outUv) noexcept {
    if (marginPixels <= 0.0F && spacingPixels <= 0.0F) {
        FillUniformGridUv(tileId, atlasTilesU, atlasTilesV, outUv);
        return;
    }
    outUv = {0.0F, 0.0F, 0.0F, 0.0F};
    if (atlasTilesU == 0 || atlasTilesV == 0 || textureWidth == 0 || textureHeight == 0 ||
        tileId == TilemapComponent::kEmptyTile) {
        return;
    }
    const std::uint32_t cells = atlasTilesU * atlasTilesV;
    if (tileId >= cells) {
        return;
    }
    const float texW = static_cast<float>(textureWidth);
    const float texH = static_cast<float>(textureHeight);
    const float margin = marginPixels >= 0.0F ? marginPixels : 0.0F;
    const float spacing = spacingPixels >= 0.0F ? spacingPixels : 0.0F;
    float tileW = 0.0F;
    float tileH = 0.0F;
    if (tilePixelWidth > 0U && tilePixelHeight > 0U) {
        tileW = static_cast<float>(tilePixelWidth);
        tileH = static_cast<float>(tilePixelHeight);
    } else {
        tileW = (texW - 2.0F * margin + spacing) / static_cast<float>(atlasTilesU) - spacing;
        tileH = (texH - 2.0F * margin + spacing) / static_cast<float>(atlasTilesV) - spacing;
    }
    if (tileW <= 0.0F || tileH <= 0.0F) {
        FillUniformGridUv(tileId, atlasTilesU, atlasTilesV, outUv);
        return;
    }
    const std::uint32_t tx = tileId % atlasTilesU;
    const std::uint32_t ty = tileId / atlasTilesU;
    const float px = margin + static_cast<float>(tx) * (tileW + spacing);
    const float py = margin + static_cast<float>(ty) * (tileH + spacing);
    outUv.x = px / texW;
    outUv.y = py / texH;
    outUv.z = (px + tileW) / texW;
    outUv.w = (py + tileH) / texH;
}

void TileIdToAtlasUvRect(
        const std::uint16_t tileId,
        const std::uint32_t atlasTilesU,
        const std::uint32_t atlasTilesV,
        const float marginPixels,
        const float spacingPixels,
        const std::uint32_t textureWidth,
        const std::uint32_t textureHeight,
        Vector4& outUv) noexcept {
    TileIdToAtlasUvRect(
            tileId,
            atlasTilesU,
            atlasTilesV,
            marginPixels,
            spacingPixels,
            textureWidth,
            textureHeight,
            0U,
            0U,
            outUv);
}

void TileIdToAtlasUvRect(const Tileset& tileset, const std::uint16_t tileId, Vector4& outUv) noexcept {
    const SharedPtr<Texture2D>& atlas = tileset.GetAtlas();
    std::uint32_t tw = tileset.GetImagePixelWidth();
    std::uint32_t th = tileset.GetImagePixelHeight();
    if (tw == 0U || th == 0U) {
        tw = atlas ? atlas->GetWidth() : 0U;
        th = atlas ? atlas->GetHeight() : 0U;
    }
    TileIdToAtlasUvRect(
            tileId,
            tileset.GetTilesU(),
            tileset.GetTilesV(),
            tileset.GetMarginPixels(),
            tileset.GetSpacingPixels(),
            tw,
            th,
            tileset.GetTilePixelWidth(),
            tileset.GetTilePixelHeight(),
            outUv);
}

}  // namespace Spark
