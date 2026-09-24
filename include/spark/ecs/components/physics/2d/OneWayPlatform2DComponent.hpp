#pragma once

#include "spark/ecs/GameComponent.hpp"

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
};

}  // namespace Spark
