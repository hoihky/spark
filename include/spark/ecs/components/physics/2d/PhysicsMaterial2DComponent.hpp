#pragma once

#include "spark/ecs/GameComponent.hpp"

namespace Spark {

/** Surface properties for 2D physics contacts; baked into <c>StaticCollider2D</c> and combined at resolve time. */
class PhysicsMaterial2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::PhysicsMaterial2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit PhysicsMaterial2DComponent(float dynamicFrictionIn = 0.48F, float restitutionIn = 0.15F) noexcept
            : dynamicFriction(dynamicFrictionIn), restitution(restitutionIn) {}

    [[nodiscard]] float GetDynamicFriction() const noexcept { return dynamicFriction; }
    void SetDynamicFriction(float v) noexcept { dynamicFriction = v; }

    [[nodiscard]] float GetRestitution() const noexcept { return restitution; }
    void SetRestitution(float v) noexcept { restitution = v; }

private:
    float dynamicFriction = 0.48F;
    float restitution = 0.15F;
};

}  // namespace Spark
