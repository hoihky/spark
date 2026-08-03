#include "spark/physics/colliders/ColliderBakePipeline3D.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/physics/colliders/ColliderBakeContext3D.hpp"
#include "spark/scene/GameWorld.hpp"

namespace Spark {

void RegisterDefaultColliderBakeStrategies3D(ColliderBakePipeline3D& pipeline);

void ColliderBakePipeline3D::ClearStrategies() noexcept {
    strategies.Clear();
}

void ColliderBakePipeline3D::RegisterStrategy(UniquePtr<IColliderBakeStrategy3D> strategy) {
    if (strategy != nullptr) {
        strategies.PushBack(MoveTemp(strategy));
    }
}

void ColliderBakePipeline3D::Rebuild(
        GameWorld& world,
        const float cellWorldSize,
        Array<Collider3D>& outColliders,
        SpatialHashGrid3D& outGrid) {
    outColliders.Clear();
    outGrid.Clear();
    outGrid.SetCellSize(cellWorldSize);

    ColliderBakeContext3D context{outColliders, outGrid};
    world.ForEachActiveGameObject([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        for (std::size_t i = 0; i < strategies.GetSize(); ++i) {
            const IColliderBakeStrategy3D& strategy = *strategies[i];
            if (strategy.Contributes(*object)) {
                strategy.Bake(*object, context);
            }
        }
    });
}

ColliderBakePipeline3D ColliderBakePipeline3D::CreateDefault() {
    ColliderBakePipeline3D pipeline{};
    RegisterDefaultColliderBakeStrategies3D(pipeline);
    return pipeline;
}

ColliderBakePipeline3D& ColliderBakePipeline3D::GetDefault() {
    static ColliderBakePipeline3D pipeline = CreateDefault();
    return pipeline;
}

}  // namespace Spark
