#pragma once

#include "spark/ai/path/IGridWalkability.hpp"
#include "spark/ai/path/GridPathfinder.hpp"
#include "spark/core/Array.hpp"
#include "spark/math/Vector2.hpp"

#include <cstdint>

namespace Spark {

/**
 * Dense blocked/walkable map for AI and gameplay queries. Implements <c>IGridWalkability</c>
 * (blocked cells are not walkable).
 */
class TilemapGameplayGrid final : public IGridWalkability {
public:
    void Resize(std::int32_t width, std::int32_t height);
    void ClearAllWalkable() noexcept;
    void SetBlocked(std::int32_t x, std::int32_t y, bool blocked) noexcept;

    [[nodiscard]] bool IsWalkable(std::int32_t x, std::int32_t y) const noexcept override;
    [[nodiscard]] std::int32_t Width() const noexcept override { return width; }
    [[nodiscard]] std::int32_t Height() const noexcept override { return height; }

    [[nodiscard]] const IGridWalkability& AsWalkability() const noexcept { return *this; }

    /** Fills <c>out</c> with cell centers on the tilemap XY plane (x = world X, y = world Y). */
    static void CellsToWorldPolylineXY(
            const Array<GridPathfinder::Cell>& cells,
            const Vector2& gridOriginXY,
            float cellSize,
            Array<Vector2>& outWorldXY);

private:
    [[nodiscard]] std::size_t CellIndex(std::int32_t x, std::int32_t y) const noexcept;

    std::int32_t width = 0;
    std::int32_t height = 0;
    Array<std::uint8_t> blocked{};
};

}  // namespace Spark
