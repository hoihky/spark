#include "spark/physics/PolygonCollider2D.hpp"

#include "spark/ecs/components/physics/2d/PolygonCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/physics/colliders/ColliderBake2D.hpp"
#include "spark/physics/PhysicsMaterial2D.hpp"
#include "spark/physics/shapes/ShapeFactory2D.hpp"

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
        Array<Collider2D>& outColliders,
        SpatialHashGrid2D& outGrid) {
    if (collider.GetVertexCount() < 3) {
        return;
    }
    ColliderFilter filter{};
    filter.categoryBits = collider.GetCategoryBits();
    filter.maskBits = collider.GetMaskBits();
    filter.isTrigger = collider.GetIsTrigger();
    ColliderMaterial material{};
    ApplyPhysicsMaterial2DToCollider(owner, material);
    UniquePtr<IShape2D> shape = ShapeFactory2D::CreateFromPolygonCollider(owner, collider);
    PushCollider2D(outColliders, outGrid, Collider2D::Create(MoveTemp(shape), filter, material, &owner));
}

}  // namespace Spark
