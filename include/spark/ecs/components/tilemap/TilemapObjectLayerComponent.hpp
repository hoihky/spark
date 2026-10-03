#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/scene/tilemap/TilemapObject.hpp"

#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;
class TilemapComponent;

/**
 * Tiled-style object layers on the sibling <c>TilemapComponent</c> (spawn points, chests, warps).
 * Does not spawn entities — pair with <c>TilemapObjectSpawnComponent</c> / gizmo component.
 */
class TilemapObjectLayerComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::TilemapObjectLayer;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] std::uint32_t GetLayerCount() const noexcept {
        return static_cast<std::uint32_t>(objectLayers.GetSize());
    }

    SPARK_SCRIPT_BIND(get_layer)
    [[nodiscard]] TilemapObjectLayer& GetLayer(const std::uint32_t layerIndex) noexcept;
    SPARK_SCRIPT_BIND(get_layer)
    [[nodiscard]] const TilemapObjectLayer& GetLayer(const std::uint32_t layerIndex) const noexcept;

    /** Appends an empty object layer. @return New layer index. */
    SPARK_SCRIPT_BIND(get_add_object_layer)
    [[nodiscard]] std::uint32_t AddObjectLayer(const char* name = "Objects");

    SPARK_SCRIPT_BIND(remove_object_layer)
    void RemoveObjectLayer(std::uint32_t layerIndex) noexcept;

    /**
     * Adds a marker to a layer and assigns a stable <c>id</c>.
     * @return Marker id (non-zero).
     */
    SPARK_SCRIPT_BIND(get_add_marker)
    [[nodiscard]] std::uint32_t AddMarker(std::uint32_t layerIndex, TilemapObjectMarker marker);

    SPARK_SCRIPT_BIND(clear_markers)
    void ClearMarkers(std::uint32_t layerIndex) noexcept;

    SPARK_SCRIPT_BIND(is_remove_marker)
    [[nodiscard]] bool RemoveMarker(std::uint32_t layerIndex, std::uint32_t markerId) noexcept;

    SPARK_SCRIPT_BIND(find_marker)
    [[nodiscard]] const TilemapObjectMarker* FindMarker(std::uint32_t markerId) const noexcept;

    void SetProperty(
            std::uint32_t layerIndex,
            std::uint32_t markerId,
            const char* key,
            const char* value) noexcept;

    [[nodiscard]] const Array<TilemapObjectLayer>& GetObjectLayers() const noexcept { return objectLayers; }

private:
    Array<TilemapObjectLayer> objectLayers{};
    std::uint32_t nextMarkerId = 1U;
};

}  // namespace Spark
