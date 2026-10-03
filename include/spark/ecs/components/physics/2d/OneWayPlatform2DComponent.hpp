#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/**
 * Marker on static platform colliders. <c>CharacterController2DComponent</c> and
 * <c>ContactResolver2D</c> treat the owner's colliders as pass-through from below unless the
 * controller is landing from above (or drop-through is not requested).
 */
class OneWayPlatform2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::OneWayPlatform2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    /** Always <c>true</c>; exists so scripts can hold a typed <c>OneWayPlatform2DComponent</c> mirror. */
    SPARK_SCRIPT_BIND(is_one_way_platform)
    [[nodiscard]] bool IsOneWayPlatform() const noexcept { return true; }
};

}  // namespace Spark
