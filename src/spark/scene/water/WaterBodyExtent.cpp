#include "spark/scene/water/WaterBodyExtent.hpp"

namespace Spark {

WaterBodyExtent WaterBodyExtent::MakeInfinitePlaceholder() noexcept {
    return WaterBodyExtent{};
}

WaterBodyExtent WaterBodyExtent::MakeFiniteLake(float halfExtentX, float halfExtentZ) noexcept {
    WaterBodyExtent extent{};
    extent.halfExtentX = halfExtentX;
    extent.halfExtentZ = halfExtentZ;
    return extent;
}

WaterBodyExtent WaterBodyExtent::MakeRiverSpline(float halfWidth, float halfLengthAlongSpline) noexcept {
    WaterBodyExtent extent{};
    extent.halfExtentX = halfWidth;
    extent.halfExtentZ = halfLengthAlongSpline;
    return extent;
}

}  // namespace Spark
