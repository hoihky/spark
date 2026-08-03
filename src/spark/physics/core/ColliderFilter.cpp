#include "spark/physics/core/ColliderFilter.hpp"

#include "spark/physics/Collision2D.hpp"
#include "spark/physics/Collision3D.hpp"
#include "spark/physics/PhysicsQueries2D.hpp"

namespace Spark {

ColliderFilter ColliderFilter::FromStaticCollider2D(const StaticCollider2D& collider) noexcept {
    ColliderFilter filter{};
    filter.categoryBits = collider.categoryBits;
    filter.maskBits = collider.maskBits;
    filter.isTrigger = collider.isTrigger;
    return filter;
}

ColliderFilter ColliderFilter::FromStaticCollider3D(const StaticCollider3DSim& collider) noexcept {
    ColliderFilter filter{};
    filter.categoryBits = CollisionFilter2D::DefaultCategory();
    filter.maskBits = CollisionFilter2D::AllLayersMask();
    filter.isTrigger = false;
    (void)collider;
    return filter;
}

ColliderFilter ColliderFilter::FromPhysicsQueryFilter2D(const PhysicsQueryFilter2D& query) noexcept {
    ColliderFilter filter{};
    filter.categoryBits = query.queryCategoryBits;
    filter.maskBits = query.queryMaskBits;
    filter.isTrigger = false;
    return filter;
}

}  // namespace Spark
