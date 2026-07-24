#include "spark/ecs/components/tilemap/TilemapObjectLayerComponent.hpp"

namespace Spark {

TilemapObjectLayer& TilemapObjectLayerComponent::GetLayer(const std::uint32_t layerIndex) noexcept {
    return objectLayers[static_cast<std::size_t>(layerIndex)];
}

const TilemapObjectLayer& TilemapObjectLayerComponent::GetLayer(const std::uint32_t layerIndex) const noexcept {
    return objectLayers[static_cast<std::size_t>(layerIndex)];
}

std::uint32_t TilemapObjectLayerComponent::AddObjectLayer(const char* name) {
    TilemapObjectLayer layer{};
    layer.name = Utf8String(name != nullptr ? name : "Objects");
    objectLayers.PushBack(layer);
    return static_cast<std::uint32_t>(objectLayers.GetSize() - 1U);
}

void TilemapObjectLayerComponent::RemoveObjectLayer(const std::uint32_t layerIndex) noexcept {
    if (layerIndex >= objectLayers.GetSize()) {
        return;
    }
    objectLayers.RemoveAt(static_cast<std::size_t>(layerIndex));
}

std::uint32_t TilemapObjectLayerComponent::AddMarker(
        const std::uint32_t layerIndex,
        TilemapObjectMarker marker) {
    if (layerIndex >= objectLayers.GetSize()) {
        return 0U;
    }
    marker.id = nextMarkerId_++;
    if (marker.id == 0U) {
        marker.id = nextMarkerId_++;
    }
    objectLayers[static_cast<std::size_t>(layerIndex)].markers.PushBack(marker);
    return marker.id;
}

void TilemapObjectLayerComponent::ClearMarkers(const std::uint32_t layerIndex) noexcept {
    if (layerIndex < objectLayers.GetSize()) {
        objectLayers[static_cast<std::size_t>(layerIndex)].markers.Clear();
    }
}

bool TilemapObjectLayerComponent::RemoveMarker(
        const std::uint32_t layerIndex,
        const std::uint32_t markerId) noexcept {
    if (layerIndex >= objectLayers.GetSize() || markerId == 0U) {
        return false;
    }
    Array<TilemapObjectMarker>& markers = objectLayers[static_cast<std::size_t>(layerIndex)].markers;
    for (std::size_t i = 0; i < markers.GetSize(); ++i) {
        if (markers[i].id == markerId) {
            markers.RemoveAt(i);
            return true;
        }
    }
    return false;
}

const TilemapObjectMarker* TilemapObjectLayerComponent::FindMarker(const std::uint32_t markerId) const noexcept {
    if (markerId == 0U) {
        return nullptr;
    }
    for (std::size_t li = 0; li < objectLayers.GetSize(); ++li) {
        const Array<TilemapObjectMarker>& markers = objectLayers[li].markers;
        for (std::size_t i = 0; i < markers.GetSize(); ++i) {
            if (markers[i].id == markerId) {
                return &markers[i];
            }
        }
    }
    return nullptr;
}

void TilemapObjectLayerComponent::SetProperty(
        const std::uint32_t layerIndex,
        const std::uint32_t markerId,
        const char* key,
        const char* value) noexcept {
    if (layerIndex >= objectLayers.GetSize() || key == nullptr) {
        return;
    }
    Array<TilemapObjectMarker>& markers = objectLayers[static_cast<std::size_t>(layerIndex)].markers;
    for (std::size_t i = 0; i < markers.GetSize(); ++i) {
        if (markers[i].id != markerId) {
            continue;
        }
        TilemapObjectMarker& marker = markers[i];
        for (std::size_t pi = 0; pi < marker.properties.GetSize(); ++pi) {
            if (marker.properties[pi].key == Utf8String(key)) {
                marker.properties[pi].value = Utf8String(value != nullptr ? value : "");
                return;
            }
        }
        TilemapObjectProperty prop{};
        prop.key = Utf8String(key);
        prop.value = Utf8String(value != nullptr ? value : "");
        marker.properties.PushBack(prop);
        return;
    }
}

}  // namespace Spark
