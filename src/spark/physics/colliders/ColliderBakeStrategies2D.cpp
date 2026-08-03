#include "spark/physics/colliders/ColliderBakePipeline2D.hpp"

#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/CircleCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/PolygonCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/physics/2d/TilemapCollider2DComponent.hpp"
#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/physics/colliders/ColliderBake2D.hpp"
#include "spark/physics/colliders/IColliderBakeStrategy2D.hpp"
#include "spark/physics/PolygonCollider2D.hpp"
#include "spark/physics/PhysicsMaterial2D.hpp"
#include "spark/physics/shapes/ShapeFactory2D.hpp"
#include "spark/physics/TilemapCollider2D.hpp"

namespace Spark {

namespace {

[[nodiscard]] bool IsNonDynamicRigidbody2D(const GameObject& object) noexcept {
    const Rigidbody2DComponent* rb = object.GetComponent<Rigidbody2DComponent>();
    if (rb == nullptr) {
        return true;
    }
    return rb->GetBodyType() != RigidbodyBodyType2D::Dynamic;
}

class TilemapColliderBakeStrategy2D final : public IColliderBakeStrategy2D {
public:
    [[nodiscard]] bool Contributes(GameObject& object) const noexcept override {
        return ContributesTilemapCollider2DStatic(object);
    }

    void Bake(GameObject& object, ColliderBakeContext2D& context) const override {
        const TilemapCollider2DComponent* tileCollider = object.GetComponent<TilemapCollider2DComponent>();
        const TilemapComponent* tilemap = object.GetComponent<TilemapComponent>();
        if (tileCollider == nullptr || tilemap == nullptr) {
            return;
        }
        AppendTilemapCollider2DStatics(object, *tileCollider, *tilemap, context.colliders, context.grid);
    }
};

class PolygonColliderBakeStrategy2D final : public IColliderBakeStrategy2D {
public:
    [[nodiscard]] bool Contributes(GameObject& object) const noexcept override {
        return ContributesPolygonCollider2DStatic(object);
    }

    void Bake(GameObject& object, ColliderBakeContext2D& context) const override {
        const PolygonCollider2DComponent* poly = object.GetComponent<PolygonCollider2DComponent>();
        if (poly == nullptr) {
            return;
        }
        AppendPolygonCollider2DStatic(object, *poly, context.colliders, context.grid);
    }
};

class BoxColliderBakeStrategy2D final : public IColliderBakeStrategy2D {
public:
    [[nodiscard]] bool Contributes(GameObject& object) const noexcept override {
        if (object.GetComponent<BoxCollider2DComponent>() == nullptr) {
            return false;
        }
        return IsNonDynamicRigidbody2D(object);
    }

    void Bake(GameObject& object, ColliderBakeContext2D& context) const override {
        const BoxCollider2DComponent* col = object.GetComponent<BoxCollider2DComponent>();
        if (col == nullptr) {
            return;
        }
        ColliderFilter filter{};
        filter.categoryBits = col->GetCategoryBits();
        filter.maskBits = col->GetMaskBits();
        filter.isTrigger = col->GetIsTrigger();
        ColliderMaterial material{};
        ApplyPhysicsMaterial2DToCollider(object, material);
        UniquePtr<IShape2D> shape = ShapeFactory2D::CreateFromBoxCollider(object, *col);
        PushCollider2D(
                context.colliders,
                context.grid,
                Collider2D::Create(MoveTemp(shape), filter, material, &object));
    }
};

class CircleColliderBakeStrategy2D final : public IColliderBakeStrategy2D {
public:
    [[nodiscard]] bool Contributes(GameObject& object) const noexcept override {
        if (object.GetComponent<CircleCollider2DComponent>() == nullptr) {
            return false;
        }
        return IsNonDynamicRigidbody2D(object);
    }

    void Bake(GameObject& object, ColliderBakeContext2D& context) const override {
        const CircleCollider2DComponent* circ = object.GetComponent<CircleCollider2DComponent>();
        if (circ == nullptr) {
            return;
        }
        ColliderFilter filter{};
        filter.categoryBits = circ->GetCategoryBits();
        filter.maskBits = circ->GetMaskBits();
        filter.isTrigger = circ->GetIsTrigger();
        ColliderMaterial material{};
        ApplyPhysicsMaterial2DToCollider(object, material);
        UniquePtr<IShape2D> shape = ShapeFactory2D::CreateFromCircleCollider(object, *circ);
        PushCollider2D(
                context.colliders,
                context.grid,
                Collider2D::Create(MoveTemp(shape), filter, material, &object));
    }
};

}  // namespace

void RegisterDefaultColliderBakeStrategies2D(ColliderBakePipeline2D& pipeline) {
    pipeline.RegisterStrategy(UniquePtr<IColliderBakeStrategy2D>(new TilemapColliderBakeStrategy2D()));
    pipeline.RegisterStrategy(UniquePtr<IColliderBakeStrategy2D>(new PolygonColliderBakeStrategy2D()));
    pipeline.RegisterStrategy(UniquePtr<IColliderBakeStrategy2D>(new BoxColliderBakeStrategy2D()));
    pipeline.RegisterStrategy(UniquePtr<IColliderBakeStrategy2D>(new CircleColliderBakeStrategy2D()));
}

}  // namespace Spark
