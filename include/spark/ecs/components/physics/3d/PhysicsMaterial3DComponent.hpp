#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/**
 * Surface properties for 3D physics contacts (static boxes and dynamic spheres). Paired with another body via
 * geometric-mean combine in <c>PhysicsWorld3D</c>. When absent on a static collider, legacy restitution-only
 * behavior is used for that surface (no extra friction model).
 */
class PhysicsMaterial3DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::PhysicsMaterial3D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit PhysicsMaterial3DComponent(
            float staticFrictionIn = 0.55F,
            float dynamicFrictionIn = 0.48F,
            float restitutionIn = 0.22F) noexcept
            : staticFriction(staticFrictionIn), dynamicFriction(dynamicFrictionIn), restitution(restitutionIn) {}

    SPARK_SCRIPT_BIND(get_static_friction)
    [[nodiscard]] float GetStaticFriction() const noexcept { return staticFriction; }
    SPARK_SCRIPT_BIND(set_static_friction)
    void SetStaticFriction(const float v) noexcept { staticFriction = v; }

    SPARK_SCRIPT_BIND(get_dynamic_friction)
    [[nodiscard]] float GetDynamicFriction() const noexcept { return dynamicFriction; }
    SPARK_SCRIPT_BIND(set_dynamic_friction)
    void SetDynamicFriction(const float v) noexcept { dynamicFriction = v; }

    /** Normal-direction elasticity in [0,1]. */
    SPARK_SCRIPT_BIND(get_restitution)
    [[nodiscard]] float GetRestitution() const noexcept { return restitution; }
    SPARK_SCRIPT_BIND(set_restitution)
    void SetRestitution(const float v) noexcept { restitution = v; }

private:
    float staticFriction = 0.55F;
    float dynamicFriction = 0.48F;
    float restitution = 0.22F;
};

}  // namespace Spark
