#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Frustum.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdint>

namespace Spark {

/**
 * View-frustum culling for 2D sprites and tilemaps at scene-submit time.
 * Tilemaps use chunk tests (see <c>kDefaultTilemapChunkSize</c>) before per-tile collection.
 */
class SceneSpriteTileCull {
public:
    static constexpr std::uint32_t kDefaultTilemapChunkSize = 16U;

    explicit SceneSpriteTileCull(const Matrix4& viewProjection) noexcept;

    [[nodiscard]] bool IsAxisAlignedBoxVisible(const Vector3& boxMin, const Vector3& boxMax) const noexcept;

    /** Unit sprite quad is XY [-0.5,0.5]² at z=0 in model space. */
    [[nodiscard]] bool IsSpriteVisible(const Matrix4& spriteModel) const noexcept;

    static void ComputeSpriteWorldBounds(const Matrix4& spriteModel, Vector3& outMin, Vector3& outMax) noexcept;

    static void ComputeTilemapWorldBounds(
            const Matrix4& world,
            float tileWorldSize,
            std::uint32_t mapWidth,
            std::uint32_t mapHeight,
            Vector3& outMin,
            Vector3& outMax) noexcept;

    /**
     * Appends visible non-empty tiles from <c>tilemap</c> into <c>outTiles</c>.
     * Stops when <c>maxTiles</c> would be exceeded.
     *
     * @return Number of tiles appended.
     */
    std::uint32_t CollectVisibleTiles(
            const Matrix4& world,
            float tileWorldSize,
            const TilemapComponent& tilemap,
            Array<SceneTilemapTileInstance>& outTiles,
            std::uint32_t maxTiles) const noexcept;

private:
    Frustum frustum{};
};

}  // namespace Spark
