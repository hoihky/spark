#pragma once

#include "spark/memory/UniquePtr.hpp"
#include "spark/physics/Collision2D.hpp"
#include "spark/physics/core/ColliderFilter.hpp"
#include "spark/physics/core/ColliderMaterial.hpp"
#include "spark/physics/shapes/IShape2D.hpp"
#include "spark/physics/shapes/ShapeType2D.hpp"

namespace Spark {

class GameObject;

/**
 * Object-oriented 2D collider snapshot: owns an <c>IShape2D</c> plus filter, material, and ECS owner.
 * Replaces bare <c>StaticCollider2D</c> POD entries in broad-phase arrays (Phase 2).
 */
class Collider2D {
public:
    Collider2D() = default;
    Collider2D(Collider2D&&) noexcept = default;
    Collider2D& operator=(Collider2D&&) noexcept = default;
    Collider2D(const Collider2D&) = delete;
    Collider2D& operator=(const Collider2D&) = delete;

    static Collider2D Create(
            UniquePtr<IShape2D> shapeIn,
            ColliderFilter filterIn,
            ColliderMaterial materialIn,
            GameObject* ownerIn) noexcept;

    /** Builds a collider from the legacy baked POD format (used during migration). */
    static Collider2D FromLegacySnapshot(const StaticCollider2D& snapshot);

    [[nodiscard]] StaticCollider2D ToLegacySnapshot() const;

    [[nodiscard]] bool IsValid() const noexcept { return shape != nullptr; }
    [[nodiscard]] const IShape2D& GetShape() const noexcept { return *shape; }
    [[nodiscard]] IShape2D& GetShape() noexcept { return *shape; }
    [[nodiscard]] ShapeType2D GetShapeType() const noexcept;
    [[nodiscard]] CollisionAabb2 GetBounds() const noexcept;

    [[nodiscard]] const ColliderFilter& GetFilter() const noexcept { return filter; }
    [[nodiscard]] ColliderFilter& GetFilter() noexcept { return filter; }
    [[nodiscard]] const ColliderMaterial& GetMaterial() const noexcept { return material; }
    [[nodiscard]] ColliderMaterial& GetMaterial() noexcept { return material; }
    [[nodiscard]] GameObject* GetOwner() const noexcept { return owner; }

    [[nodiscard]] bool IsTrigger() const noexcept { return filter.isTrigger; }
    [[nodiscard]] std::uint16_t GetCategoryBits() const noexcept { return filter.categoryBits; }
    [[nodiscard]] std::uint16_t GetMaskBits() const noexcept { return filter.maskBits; }

    [[nodiscard]] bool OverlapsAabb(const CollisionAabb2& aabb) const;
    [[nodiscard]] bool OverlapsCircle(float centerX, float centerY, float radius) const;

private:
    UniquePtr<IShape2D> shape;
    ColliderFilter filter{};
    ColliderMaterial material{};
    GameObject* owner = nullptr;
};

}  // namespace Spark
