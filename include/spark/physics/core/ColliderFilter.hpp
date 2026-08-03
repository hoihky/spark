#pragma once

#include "spark/physics/CollisionFilter2D.hpp"

#include <cstdint>

namespace Spark {

struct PhysicsQueryFilter2D;
struct StaticCollider2D;
struct StaticCollider3DSim;

/**
 * Layer bitmask filter shared by 2D and 3D colliders. Uses the same mutual-mask rule as
 * <c>CollisionFilter2D::ShouldCollide</c>.
 */
struct ColliderFilter {
    std::uint16_t categoryBits = CollisionFilter2D::DefaultCategory();
    std::uint16_t maskBits = CollisionFilter2D::AllLayersMask();
    /** When true, this collider is a trigger (overlap only, no blocking resolution). */
    bool isTrigger = false;

    [[nodiscard]] bool ShouldCollideWith(const ColliderFilter& other) const noexcept {
        return CollisionFilter2D::ShouldCollide(categoryBits, maskBits, other.categoryBits, other.maskBits);
    }

    [[nodiscard]] bool PassesQueryFilter(
            std::uint16_t queryCategoryBits,
            std::uint16_t queryMaskBits,
            bool queryHitSolids,
            bool queryHitTriggers) const noexcept {
        if (!CollisionFilter2D::ShouldCollide(queryCategoryBits, queryMaskBits, categoryBits, maskBits)) {
            return false;
        }
        if (isTrigger && !queryHitTriggers) {
            return false;
        }
        if (!isTrigger && !queryHitSolids) {
            return false;
        }
        return true;
    }

    static ColliderFilter FromStaticCollider2D(const StaticCollider2D& collider) noexcept;
    static ColliderFilter FromStaticCollider3D(const StaticCollider3DSim& collider) noexcept;
    static ColliderFilter FromPhysicsQueryFilter2D(const PhysicsQueryFilter2D& query) noexcept;
};

}  // namespace Spark
