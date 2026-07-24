#include "spark/scene/tilemap/TilemapObjectQuery.hpp"

#include "spark/ai/path/GridPathfinder.hpp"
#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/GameObject.hpp"

namespace Spark {

Vector3 TilemapObjectMarkerWorldPosition(
        const TilemapObjectMarker& marker,
        const TilemapGridFrame& frame) noexcept {
    const Vector3 local{
            (static_cast<float>(marker.cellX) + marker.offsetX) * frame.cellSize,
            (static_cast<float>(marker.cellY) + marker.offsetY) * frame.cellSize,
            0.0F};
    return frame.worldFromLocal.TransformPoint(local);
}

TilemapGridFrame MakeTilemapGridFrameForObject(
        const GameObject& mapOwner,
        const TilemapComponent& tilemap) noexcept {
    return MakeTilemapGridFrame(
            mapOwner.GetWorldMatrix(),
            tilemap.GetTileWorldSize(),
            tilemap.GetMapWidth(),
            tilemap.GetMapHeight());
}

void CollectTilemapObjectMarkersAtCell(
        const Array<TilemapObjectLayer>& layers,
        const std::uint32_t layerIndex,
        const GridPathfinder::Cell& cell,
        Array<const TilemapObjectMarker*>& outMarkers) {
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

void CollectTilemapObjectMarkersByType(
        const Array<TilemapObjectLayer>& layers,
        const Utf8String& typeId,
        Array<const TilemapObjectMarker*>& outMarkers) {
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

const TilemapObjectProperty* FindTilemapObjectProperty(
        const TilemapObjectMarker& marker,
        const char* key) noexcept {
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
