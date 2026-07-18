#include "spark/physics/PolygonCollider2D.hpp"

#include "spark/ecs/components/physics/2d/PolygonCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/physics/Collision2D.hpp"
#include "spark/physics/PhysicsMaterial2D.hpp"
#include "spark/physics/SpatialHashGrid2D.hpp"

namespace Spark {

bool ContributesPolygonCollider2DStatic(GameObject& object) noexcept {
    const PolygonCollider2DComponent* poly = object.GetComponent<PolygonCollider2DComponent>();
    if (poly == nullptr || poly->GetVertexCount() < 3) {
        return false;
    }
    const Rigidbody2DComponent* rb = object.GetComponent<Rigidbody2DComponent>();
    if (rb == nullptr) {
        return true;
    }
    return rb->GetBodyType() != RigidbodyBodyType2D::Dynamic;
}

void AppendPolygonCollider2DStatic(
        GameObject& owner,
        const PolygonCollider2DComponent& collider,
        Array<StaticCollider2D>& outStatics,
        SpatialHashGrid2D& outGrid) {
    if (collider.GetVertexCount() < 3) {
        return;
    }
    StaticCollider2D sc{};
    sc.shape = StaticCollider2DShape::ConvexPolygon;
    sc.categoryBits = collider.GetCategoryBits();
    sc.maskBits = collider.GetMaskBits();
    sc.owner = &owner;
    sc.isTrigger = collider.GetIsTrigger();
    ComputePolygonCollider2DWorld(owner, collider, sc);
    ApplyPhysicsMaterial2DToStaticRecord(owner, sc);
    const std::uint32_t idx = static_cast<std::uint32_t>(outStatics.GetSize());
    outStatics.PushBack(sc);
    outGrid.InsertIndexedAabb(idx, sc.aabb);
}

}  // namespace Spark
