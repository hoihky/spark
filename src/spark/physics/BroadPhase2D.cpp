#include "spark/physics/BroadPhase2D.hpp"

#include "spark/physics/colliders/ColliderBakePipeline2D.hpp"
#include "spark/scene/GameWorld.hpp"

namespace Spark {

void BroadPhase2D::Rebuild(GameWorld& world, const float cellWorldSize) {
    ColliderBakePipeline2D::GetDefault().Rebuild(world, cellWorldSize, colliders, grid);
}

void BroadPhase2D::Clear() noexcept {
    colliders.Clear();
    grid.Clear();
}

}  // namespace Spark
