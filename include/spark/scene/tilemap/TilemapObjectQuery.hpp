#pragma once

#include "spark/core/Array.hpp"
#include "spark/ai/path/GridPathfinder.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/tilemap/TilemapObject.hpp"
#include "spark/scene/tilemap/TilemapGridCoordinates.hpp"

namespace Spark {

/** Object-layer queries against a <c>TilemapGridFrame</c>. */
class TilemapObjectQuery final {
public:
    [[nodiscard]] Vector3 MarkerWorldPosition(
            const TilemapObjectMarker& marker,
            const TilemapGridFrame& frame) const noexcept;

    void CollectMarkersAtCell(
            const Array<TilemapObjectLayer>& layers,
            std::uint32_t layerIndex,
            const GridPathfinder::Cell& cell,
            Array<const TilemapObjectMarker*>& outMarkers) const;

    void CollectMarkersByType(
            const Array<TilemapObjectLayer>& layers,
            const Utf8String& typeId,
            Array<const TilemapObjectMarker*>& outMarkers) const;

    [[nodiscard]] const TilemapObjectProperty* FindProperty(
            const TilemapObjectMarker& marker,
            const char* key) const noexcept;
};

}  // namespace Spark
