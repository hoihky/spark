#include "spark/scene/tilemap/TilemapObjectQuery.hpp"

#include "spark/ai/path/GridPathfinder.hpp"

namespace Spark {

Vector3 TilemapObjectQuery::MarkerWorldPosition(
        const TilemapObjectMarker& marker,
        const TilemapGridFrame& frame) const noexcept {
    const Vector3 local{
            (static_cast<float>(marker.cellX) + marker.offsetX) * frame.cellSize,
            (static_cast<float>(marker.cellY) + marker.offsetY) * frame.cellSize,
            0.0F};
    return frame.worldFromLocal.TransformPoint(local);
}

void TilemapObjectQuery::CollectMarkersAtCell(
        const Array<TilemapObjectLayer>& layers,
        const std::uint32_t layerIndex,
        const GridPathfinder::Cell& cell,
        Array<const TilemapObjectMarker*>& outMarkers) const {
    if (layerIndex >= layers.GetSize() || !layers[layerIndex].visible) {
        return;
    }
    const Array<TilemapObjectMarker>& markers = layers[layerIndex].markers;
    for (std::size_t i = 0; i < markers.GetSize(); ++i) {
        if (markers[i].cellX == cell.x && markers[i].cellY == cell.y) {
            outMarkers.PushBack(&markers[i]);
        }
    }
}

void TilemapObjectQuery::CollectMarkersByType(
        const Array<TilemapObjectLayer>& layers,
        const Utf8String& typeId,
        Array<const TilemapObjectMarker*>& outMarkers) const {
    for (std::size_t li = 0; li < layers.GetSize(); ++li) {
        if (!layers[li].visible) {
            continue;
        }
        const Array<TilemapObjectMarker>& markers = layers[li].markers;
        for (std::size_t i = 0; i < markers.GetSize(); ++i) {
            if (markers[i].typeId == typeId) {
                outMarkers.PushBack(&markers[i]);
            }
        }
    }
}

const TilemapObjectProperty* TilemapObjectQuery::FindProperty(
        const TilemapObjectMarker& marker,
        const char* key) const noexcept {
    if (key == nullptr) {
        return nullptr;
    }
    for (std::size_t i = 0; i < marker.properties.GetSize(); ++i) {
        if (marker.properties[i].key == Utf8String(key)) {
            return &marker.properties[i];
        }
    }
    return nullptr;
}

}  // namespace Spark
