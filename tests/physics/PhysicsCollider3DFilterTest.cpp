#include <gtest/gtest.h>

#include "spark/ecs/components/physics/3d/BoxCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/PhysicsLayer3DComponent.hpp"
#include "spark/physics/CollisionFilter3D.hpp"
#include "spark/physics/colliders/PhysicsCollider3DFilter.hpp"
#include "spark/scene/core/GameWorld.hpp"

TEST(PhysicsCollider3DFilterTest, BoxColliderBitsOverrideLayerComponent) {
    Spark::GameWorld world{};
    Spark::GameObject* object = world.CreateGameObject();
    object->AddComponent<Spark::PhysicsLayer3DComponent>()->SetCategoryBits(0x00FFu);
    auto* box = object->AddComponent<Spark::BoxCollider3DComponent>();
    box->SetCategoryBits(Spark::CollisionFilter3D::LayerBit(Spark::PhysicsLayers3D::Enemy));

    const Spark::ColliderFilter filter = Spark::PhysicsCollider3DFilter::Resolve(*object);
    EXPECT_EQ(filter.categoryBits, Spark::CollisionFilter3D::LayerBit(Spark::PhysicsLayers3D::Enemy));
}

TEST(PhysicsCollider3DFilterTest, LayerComponentUsedWhenNoShapeCollider) {
    Spark::GameWorld world{};
    Spark::GameObject* object = world.CreateGameObject();
    auto* layer = object->AddComponent<Spark::PhysicsLayer3DComponent>();
    layer->SetCategoryBits(Spark::CollisionFilter3D::LayerBit(Spark::PhysicsLayers3D::Environment));

    const Spark::ColliderFilter filter = Spark::PhysicsCollider3DFilter::Resolve(*object);
    EXPECT_EQ(filter.categoryBits, Spark::CollisionFilter3D::LayerBit(Spark::PhysicsLayers3D::Environment));
}
