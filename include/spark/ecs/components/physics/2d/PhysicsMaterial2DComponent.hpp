#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/** Surface properties for 2D physics contacts; baked into <c>StaticCollider2D</c> and combined at resolve time. */
class PhysicsMaterial2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::PhysicsMaterial2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit PhysicsMaterial2DComponent(float dynamicFrictionIn = 0.48F, float restitutionIn = 0.15F) noexcept
            : dynamicFriction(dynamicFrictionIn), restitution(restitutionIn) {}

    SPARK_SCRIPT_BIND(get_dynamic_friction)
    [[nodiscard]] float GetDynamicFriction() const noexcept { return dynamicFriction; }
    SPARK_SCRIPT_BIND(set_dynamic_friction)
    void SetDynamicFriction(float v) noexcept { dynamicFriction = v; }

    SPARK_SCRIPT_BIND(get_restitution)
    [[nodiscard]] float GetRestitution() const noexcept { return restitution; }
    SPARK_SCRIPT_BIND(set_restitution)
    void SetRestitution(float v) noexcept { restitution = v; }

private:
    float dynamicFriction = 0.48F;
    float restitution = 0.15F;
};

}  // namespace Spark
