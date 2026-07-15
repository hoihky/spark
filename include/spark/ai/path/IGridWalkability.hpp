#pragma once

#include <cstdint>

namespace Spark {

/** Abstract occupancy query for grid path-finding (Dependency inversion over concrete tilemaps). */
class IGridWalkability {
public:
    virtual ~IGridWalkability() = default;

    [[nodiscard]] virtual bool IsWalkable(std::int32_t x, std::int32_t y) const noexcept = 0;
    [[nodiscard]] virtual std::int32_t Width() const noexcept = 0;
    [[nodiscard]] virtual std::int32_t Height() const noexcept = 0;
};

}  // namespace Spark
