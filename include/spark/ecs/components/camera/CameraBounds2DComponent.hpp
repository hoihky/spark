#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/physics/Collision2D.hpp"

#include <cstdint>

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

    void SetEnabled(const bool value) noexcept { enabled = value; }
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    void SetLocalOffset(const Vector2& offset) noexcept { localOffset = offset; }
    [[nodiscard]] const Vector2& GetLocalOffset() const noexcept { return localOffset; }

    void SetHalfExtents(const Vector2& value) noexcept;
    [[nodiscard]] const Vector2& GetHalfExtents() const noexcept { return halfExtents; }

    void SetPriority(const int value) noexcept { priority = value; }
    [[nodiscard]] int GetPriority() const noexcept { return priority; }

    /** Computes axis-aligned bounds in world XY (ignores Z). */
    void ComputeWorldBounds(const GameObject& owner, CollisionAabb2& out) const noexcept;

    /** Returns true when <c>worldX/worldY</c> lies inside this volume. */
    [[nodiscard]] bool ContainsWorldPoint(const GameObject& owner, float worldX, float worldY) const noexcept;

private:
    Vector2 localOffset{Vector2::Zero};
    Vector2 halfExtents{10.0F, 6.0F};
    int priority = 0;
    bool enabled = true;
};

}  // namespace Spark
