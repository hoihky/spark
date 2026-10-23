#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/scene/submit/RenderLayerId.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/**
 * Named sorting layer + optional order-in-layer for 2D drawables on the same object.
 * When absent, submit uses the <c>Default</c> layer and the drawable's native sort field.
 *
 * Pair with <c>SpriteComponent</c> or <c>TilemapComponent</c> (each stacked <c>TilemapLayer</c> is a separate draw).
 * Order-in-layer is explicit only when <c>SetOrderInLayer</c> was called; otherwise
 * <c>SpriteComponent::sortOrder</c> / tilemap base order (+ per-layer offsets) apply.
 */
class RenderLayerComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::RenderLayer;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    RenderLayerComponent() = default;
    explicit RenderLayerComponent(RenderLayerId layerIdIn) noexcept : layerId(layerIdIn) {}
    RenderLayerComponent(RenderLayerId layerIdIn, std::int32_t orderInLayerIn) noexcept
            : layerId(layerIdIn), orderInLayer(orderInLayerIn), explicitOrderInLayer(true) {}

    SPARK_SCRIPT_BIND(get_layer_id)
    [[nodiscard]] RenderLayerId GetLayerId() const noexcept { return layerId; }
    SPARK_SCRIPT_BIND(get_order_in_layer)
    [[nodiscard]] std::int32_t GetOrderInLayer() const noexcept { return orderInLayer; }
    SPARK_SCRIPT_BIND(has_s_explicit_order_in_layer)
    [[nodiscard]] bool HasExplicitOrderInLayer() const noexcept { return explicitOrderInLayer; }

    SPARK_SCRIPT_BIND(set_layer_id)
    void SetLayerId(RenderLayerId id) noexcept { layerId = id; }
    void SetOrderInLayer(std::int32_t order) noexcept {
        orderInLayer = order;
        explicitOrderInLayer = true;
    }
    SPARK_SCRIPT_BIND(clear_explicit_order_in_layer)
    void ClearExplicitOrderInLayer() noexcept { explicitOrderInLayer = false; }

private:
    RenderLayerId layerId = 0;
    std::int32_t orderInLayer = 0;
    bool explicitOrderInLayer = false;
};

}  // namespace Spark
