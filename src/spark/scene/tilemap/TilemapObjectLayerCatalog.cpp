#include "spark/scene/tilemap/TilemapObjectLayerCatalog.hpp"

#include "spark/ecs/components/tilemap/TilemapObjectLayerComponent.hpp"
#include "spark/scene/tilemap/TilemapObject.hpp"

#include <cstring>

namespace Spark {

TilemapObjectLayerCatalog::TilemapObjectLayerCatalog(const TilemapObjectLayerComponent& objectLayers) noexcept
        : layers(objectLayers) {}

namespace {

[[nodiscard]] bool TypeIdMatches(const Utf8String& markerType, const char* typeId) noexcept {
    if (typeId == nullptr || typeId[0] == '\0') {
        return false;
    }
    return std::strcmp(markerType.CStr(), typeId) == 0;
}

}  // namespace

std::size_t TilemapObjectLayerCatalog::CountMarkersByTypeId(const char* const typeId) const noexcept {
    std::size_t count = 0U;
    const Array<TilemapObjectLayer>& objectLayers = layers.GetObjectLayers();
    for (std::size_t li = 0; li < objectLayers.GetSize(); ++li) {
        const Array<TilemapObjectMarker>& markers = objectLayers[li].markers;
        for (std::size_t mi = 0; mi < markers.GetSize(); ++mi) {
            if (TypeIdMatches(markers[mi].typeId, typeId)) {
                ++count;
            }
        }
    }
    return count;
}

bool TilemapObjectLayerCatalog::ContainsTypeId(const char* const typeId) const noexcept {
    return CountMarkersByTypeId(typeId) > 0U;
}

std::size_t TilemapObjectLayerCatalog::TotalMarkerCount() const noexcept {
    std::size_t count = 0U;
    const Array<TilemapObjectLayer>& objectLayers = layers.GetObjectLayers();
    for (std::size_t li = 0; li < objectLayers.GetSize(); ++li) {
        count += objectLayers[li].markers.GetSize();
    }
    return count;
}

}  // namespace Spark
