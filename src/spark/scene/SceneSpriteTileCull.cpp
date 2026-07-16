#include "spark/scene/SceneSpriteTileCull.hpp"

#include "spark/math/AxisAlignedBox.hpp"

#include <algorithm>

namespace Spark {

SceneSpriteTileCull::SceneSpriteTileCull(const Matrix4& viewProjection) noexcept
    : frustum(Frustum::FromColumnMajorViewProjection(viewProjection)) {}

bool SceneSpriteTileCull::IsAxisAlignedBoxVisible(const Vector3& boxMin, const Vector3& boxMax) const noexcept {
    return frustum.IntersectsAxisAlignedBox(boxMin, boxMax);
}

void SceneSpriteTileCull::ComputeSpriteWorldBounds(
        const Matrix4& spriteModel, Vector3& outMin, Vector3& outMax) noexcept {
    static constexpr Vector3 kLocalMin{-0.5F, -0.5F, 0.0F};
    static constexpr Vector3 kLocalMax{0.5F, 0.5F, 0.0F};
    AxisAlignedBox::EncapsulateTransformedLocal(kLocalMin, kLocalMax, spriteModel, outMin, outMax);
}

bool SceneSpriteTileCull::IsSpriteVisible(const Matrix4& spriteModel) const noexcept {
    Vector3 bmin{};
    Vector3 bmax{};
    ComputeSpriteWorldBounds(spriteModel, bmin, bmax);
    return IsAxisAlignedBoxVisible(bmin, bmax);
}

void SceneSpriteTileCull::ComputeTilemapWorldBounds(
        const Matrix4& world,
        const float tileWorldSize,
        const std::uint32_t mapWidth,
        const std::uint32_t mapHeight,
        Vector3& outMin,
        Vector3& outMax) noexcept {
    const float w = static_cast<float>(mapWidth) * tileWorldSize;
    const float h = static_cast<float>(mapHeight) * tileWorldSize;
    static constexpr float kDepthPad = 0.05F;
    const Vector3 localMin{0.0F, 0.0F, -kDepthPad};
    const Vector3 localMax{w, h, kDepthPad};
    AxisAlignedBox::EncapsulateTransformedLocal(localMin, localMax, world, outMin, outMax);
}

std::uint32_t SceneSpriteTileCull::CollectVisibleTiles(
        const Matrix4& world,
        const float tileWorldSize,
        const TilemapComponent& tilemap,
        Array<SceneTilemapTileInstance>& outTiles,
        const std::uint32_t maxTiles) const noexcept {
    const std::uint32_t mw = tilemap.GetMapWidth();
    const std::uint32_t mh = tilemap.GetMapHeight();
    if (mw == 0 || mh == 0 || tileWorldSize <= 0.0F) {
        return 0;
    }

    Vector3 mapMin{};
    Vector3 mapMax{};
    ComputeTilemapWorldBounds(world, tileWorldSize, mw, mh, mapMin, mapMax);
    if (!IsAxisAlignedBoxVisible(mapMin, mapMax)) {
        return 0;
    }

    std::uint32_t appended = 0;
    const std::uint32_t chunkSize = kDefaultTilemapChunkSize;
    const std::uint32_t chunksX = (mw + chunkSize - 1U) / chunkSize;
    const std::uint32_t chunksY = (mh + chunkSize - 1U) / chunkSize;
    static constexpr float kChunkDepthPad = 0.05F;

    for (std::uint32_t cy = 0; cy < chunksY; ++cy) {
        for (std::uint32_t cx = 0; cx < chunksX; ++cx) {
            const std::uint32_t x0 = cx * chunkSize;
            const std::uint32_t y0 = cy * chunkSize;
            const std::uint32_t x1 = std::min(x0 + chunkSize, mw);
            const std::uint32_t y1 = std::min(y0 + chunkSize, mh);

            const Vector3 chunkLocalMin{
                    static_cast<float>(x0) * tileWorldSize,
                    static_cast<float>(y0) * tileWorldSize,
                    -kChunkDepthPad};
            const Vector3 chunkLocalMax{
                    static_cast<float>(x1) * tileWorldSize,
                    static_cast<float>(y1) * tileWorldSize,
                    kChunkDepthPad};
            Vector3 chunkWorldMin{};
            Vector3 chunkWorldMax{};
            AxisAlignedBox::EncapsulateTransformedLocal(chunkLocalMin, chunkLocalMax, world, chunkWorldMin, chunkWorldMax);
            if (!IsAxisAlignedBoxVisible(chunkWorldMin, chunkWorldMax)) {
                continue;
            }

            for (std::uint32_t iy = y0; iy < y1; ++iy) {
                for (std::uint32_t ix = x0; ix < x1; ++ix) {
                    if (outTiles.GetSize() >= static_cast<std::size_t>(maxTiles)) {
                        return appended;
                    }
                    const std::uint16_t tid = tilemap.GetTile(ix, iy);
                    if (tid == TilemapComponent::kEmptyTile) {
                        continue;
                    }
                    SceneTilemapTileInstance inst{};
                    inst.gridX = static_cast<std::uint16_t>(ix);
                    inst.gridY = static_cast<std::uint16_t>(iy);
                    inst.tileId = tid;
                    outTiles.PushBack(inst);
                    ++appended;
                }
            }
        }
    }
    return appended;
}

}  // namespace Spark
