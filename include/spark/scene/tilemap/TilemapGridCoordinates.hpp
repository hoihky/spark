#pragma once

#include "spark/ai/path/GridPathfinder.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdint>

namespace Spark {

/**
 * World ↔ grid mapping for a tilemap on the XY plane (local origin at cell corner (0,0)).
 */
struct TilemapGridFrame {
    Matrix4 worldFromLocal = Matrix4::Identity;
    float cellSize = 1.0F;
    std::uint32_t mapWidth = 0;
    std::uint32_t mapHeight = 0;

    [[nodiscard]] Vector2 LocalCornerToWorldXY() const noexcept;
    [[nodiscard]] Vector2 CellCenterToWorldXY(const GridPathfinder::Cell& cell) const noexcept;
    [[nodiscard]] GridPathfinder::Cell WorldXYToCell(const Vector2& worldXY) const noexcept;
    [[nodiscard]] GridPathfinder::Cell WorldPositionToCell(const Vector3& worldPosition) const noexcept;
    [[nodiscard]] bool IsCellInBounds(const GridPathfinder::Cell& cell) const noexcept;
};

/** Builds a frame from a tilemap object's world matrix and map dimensions. */
[[nodiscard]] TilemapGridFrame MakeTilemapGridFrame(
        const Matrix4& tilemapWorldMatrix,
        float tileWorldSize,
        std::uint32_t mapWidth,
        std::uint32_t mapHeight) noexcept;

}  // namespace Spark
