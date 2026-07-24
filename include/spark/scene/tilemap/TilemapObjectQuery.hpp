#pragma once

#include "spark/core/Array.hpp"
#include "spark/ai/path/GridPathfinder.hpp"
#include "spark/scene/tilemap/TilemapObject.hpp"
#include "spark/scene/tilemap/TilemapGridCoordinates.hpp"

namespace Spark {

class GameObject;
class TilemapComponent;

/** World position of a marker using the map grid frame. */
[[nodiscard]] Vector3 TilemapObjectMarkerWorldPosition(
        const TilemapObjectMarker& marker,
        const TilemapGridFrame& frame) noexcept;

[[nodiscard]] TilemapGridFrame MakeTilemapGridFrameForObject(
        const GameObject& mapOwner,
        const TilemapComponent& tilemap) noexcept;

/** Appends markers on <c>layerIndex</c> whose cell matches <c>cell</c>. */
void CollectTilemapObjectMarkersAtCell(
        const Array<TilemapObjectLayer>& layers,
        std::uint32_t layerIndex,
        const GridPathfinder::Cell& cell,
        Array<const TilemapObjectMarker*>& outMarkers);

/** Appends markers whose <c>typeId</c> matches (UTF-8 exact). */
void CollectTilemapObjectMarkersByType(
        const Array<TilemapObjectLayer>& layers,
        const Utf8String& typeId,
        Array<const TilemapObjectMarker*>& outMarkers);

[[nodiscard]] const TilemapObjectProperty* FindTilemapObjectProperty(
        const TilemapObjectMarker& marker,
        const char* key) noexcept;

}  // namespace Spark
