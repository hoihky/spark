#pragma once

#include "spark/core/Array.hpp"
#include "spark/math/Vector2.hpp"

#include <cstdint>

namespace Spark {

/** Visibility state for one gameplay grid cell. */
enum class FogOfWarCellState : std::uint8_t {
    Hidden = 0,
    Explored = 1,
    Visible = 2,
};

/** Grid-backed exploration model (Memento-friendly: only cell states, no GPU handles). */
class FogOfWarExplorationMap final {
public:
    void Reset(const std::int32_t width, const std::int32_t height) noexcept;

    [[nodiscard]] std::int32_t GetWidth() const noexcept { return mapWidth; }
    [[nodiscard]] std::int32_t GetHeight() const noexcept { return mapHeight; }

    void RevealDisc(const Vector2& worldCenter, const float worldRadius, const class TilemapGridFrame& frame) noexcept;

    [[nodiscard]] FogOfWarCellState GetCellState(const std::int32_t x, const std::int32_t y) const noexcept;

private:
    void SetCellState(const std::int32_t x, const std::int32_t y, const FogOfWarCellState state) noexcept;

    std::int32_t mapWidth = 0;
    std::int32_t mapHeight = 0;
    Array<FogOfWarCellState> cells{};
};

}  // namespace Spark
