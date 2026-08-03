#include "spark/physics/colliders/ColliderBakePipeline2D.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/physics/colliders/ColliderBakeContext2D.hpp"
#include "spark/scene/GameWorld.hpp"

namespace Spark {

void RegisterDefaultColliderBakeStrategies2D(ColliderBakePipeline2D& pipeline);

void ColliderBakePipeline2D::ClearStrategies() noexcept {
    strategies.Clear();
}

void ColliderBakePipeline2D::RegisterStrategy(UniquePtr<IColliderBakeStrategy2D> strategy) {
    if (strategy != nullptr) {
        strategies.PushBack(MoveTemp(strategy));
    }
}

void ColliderBakePipeline2D::Rebuild(
        GameWorld& world,
        const float cellWorldSize,
        Array<Collider2D>& outColliders,
        SpatialHashGrid2D& outGrid) {
    outColliders.Clear();
    outGrid.Clear();
    outGrid.SetCellSize(cellWorldSize);

    ColliderBakeContext2D context{outColliders, outGrid};
    world.ForEachActiveGameObject([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        for (std::size_t i = 0; i < strategies.GetSize(); ++i) {
            const IColliderBakeStrategy2D& strategy = *strategies[i];
            if (strategy.Contributes(*object)) {
                strategy.Bake(*object, context);
            }
        }
    });
}

ColliderBakePipeline2D ColliderBakePipeline2D::CreateDefault() {
    ColliderBakePipeline2D pipeline{};
    RegisterDefaultColliderBakeStrategies2D(pipeline);
    return pipeline;
}

ColliderBakePipeline2D& ColliderBakePipeline2D::GetDefault() {
    static ColliderBakePipeline2D pipeline = CreateDefault();
    return pipeline;
}

}  // namespace Spark
