#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/physics/Collision2D.hpp"

#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;

/**
 * World-space rectangle that can drive a <c>Camera2DRigComponent</c> follow bounds.
 * When the rig target lies inside multiple zones, the highest <c>priority</c> wins.
 */
class CameraBounds2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::CameraBounds2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    SPARK_SCRIPT_BIND(set_enabled)
    void SetEnabled(const bool value) noexcept { enabled = value; }
    SPARK_SCRIPT_BIND(is_enabled)
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    SPARK_SCRIPT_BIND(set_local_offset)
    void SetLocalOffset(const Vector2& offset) noexcept { localOffset = offset; }
    SPARK_SCRIPT_BIND(get_local_offset)
    [[nodiscard]] const Vector2& GetLocalOffset() const noexcept { return localOffset; }

    SPARK_SCRIPT_BIND(set_half_extents)
    void SetHalfExtents(const Vector2& value) noexcept;
    SPARK_SCRIPT_BIND(get_half_extents)
    [[nodiscard]] const Vector2& GetHalfExtents() const noexcept { return halfExtents; }

    SPARK_SCRIPT_BIND(set_priority)
    void SetPriority(const int value) noexcept { priority = value; }
    SPARK_SCRIPT_BIND(get_priority)
    [[nodiscard]] int GetPriority() const noexcept { return priority; }

    /** Computes axis-aligned bounds in world XY (ignores Z). */
    SPARK_SCRIPT_BIND(compute_world_bounds)
    void ComputeWorldBounds(const GameObject& owner, CollisionAabb2& out) const noexcept;

    /** Returns true when <c>worldX/worldY</c> lies inside this volume. */
    SPARK_SCRIPT_BIND(is_contains_world_point)
    [[nodiscard]] bool ContainsWorldPoint(const GameObject& owner, float worldX, float worldY) const noexcept;

private:
    Vector2 localOffset{Vector2::Zero};
    Vector2 halfExtents{10.0F, 6.0F};
    int priority = 0;
    bool enabled = true;
};

}  // namespace Spark
