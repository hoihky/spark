#pragma once

#include "spark/ai/path/GridPathfinder.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/scene/tilemap/TileCell.hpp"

#include <cstdint>

namespace Spark {

class GameObject;
class TilemapComponent;
class TilemapObjectLayerComponent;

/** Result of a tile cell hit test. */
class TilemapCellPick final {
public:
    [[nodiscard]] bool IsHit() const noexcept { return hit; }
    [[nodiscard]] std::uint32_t GetLayerIndex() const noexcept { return layerIndex; }
    [[nodiscard]] const GridPathfinder::Cell& GetCell() const noexcept { return cell; }
    [[nodiscard]] const TileCell& GetTile() const noexcept { return tile; }

    bool hit = false;
    std::uint32_t layerIndex = 0;
    GridPathfinder::Cell cell{};
    TileCell tile{};
};

/** Result of an object-marker hit test. */
class TilemapObjectPick final {
public:
    [[nodiscard]] bool IsHit() const noexcept { return hit; }
    [[nodiscard]] std::uint32_t GetObjectLayerIndex() const noexcept { return objectLayerIndex; }
    [[nodiscard]] std::uint32_t GetMarkerId() const noexcept { return markerId; }
    [[nodiscard]] float GetDistanceSquared() const noexcept { return distanceSquared; }

    bool hit = false;
    std::uint32_t objectLayerIndex = 0;
    std::uint32_t markerId = 0;
    float distanceSquared = 0.0F;
};

/** World-space hit testing for tilemaps (uses <c>TilemapGridFrame</c>). */
class TilemapPicker final {
public:
    [[nodiscard]] TilemapCellPick PickCellAtWorld(
            const GameObject& owner,
            const TilemapComponent& tilemap,
            const Vector2& worldXY) const noexcept;

    [[nodiscard]] TilemapCellPick PickTopmostTile(
            const GameObject& owner,
            const TilemapComponent& tilemap,
            const Vector2& worldXY) const noexcept;

    [[nodiscard]] TilemapCellPick PickCellOnLayer(
            const GameObject& owner,
            const TilemapComponent& tilemap,
            std::uint32_t layerIndex,
            const Vector2& worldXY) const noexcept;

    [[nodiscard]] TilemapObjectPick PickNearestObjectMarker(
            const GameObject& owner,
            const TilemapComponent& tilemap,
            const TilemapObjectLayerComponent& objects,
            const Vector2& worldXY,
            float maxDistanceWorld = 0.6F) const noexcept;
};

}  // namespace Spark
