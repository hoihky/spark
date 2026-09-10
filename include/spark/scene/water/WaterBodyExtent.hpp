#pragma once

namespace Spark {

/** World-space footprint for `FiniteLake` and `RiverSpline` modes (tile size for `InfiniteOcean` uses mesh settings). */
class WaterBodyExtent {
public:
    [[nodiscard]] static WaterBodyExtent MakeInfinitePlaceholder() noexcept;

    [[nodiscard]] static WaterBodyExtent MakeFiniteLake(float halfExtentX, float halfExtentZ) noexcept;

    /** v1 placeholder: half-width of the river strip; length uses `GetHalfExtentZ`. */
    [[nodiscard]] static WaterBodyExtent MakeRiverSpline(float halfWidth, float halfLengthAlongSpline) noexcept;

    [[nodiscard]] float GetHalfExtentX() const noexcept { return halfExtentX; }
    [[nodiscard]] float GetHalfExtentZ() const noexcept { return halfExtentZ; }

    void SetHalfExtentX(float value) noexcept { halfExtentX = value; }
    void SetHalfExtentZ(float value) noexcept { halfExtentZ = value; }

private:
    float halfExtentX = 64.0F;
    float halfExtentZ = 64.0F;
};

}  // namespace Spark
