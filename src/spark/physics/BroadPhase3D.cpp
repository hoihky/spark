#include "spark/physics/BroadPhase3D.hpp"

#include "spark/physics/colliders/ColliderBakePipeline3D.hpp"
#include "spark/scene/GameWorld.hpp"

namespace Spark {

void BroadPhase3D::Rebuild(GameWorld& world, const float cellWorldSize) {
    ColliderBakePipeline3D::GetDefault().Rebuild(world, cellWorldSize, colliders, grid);
}

void BroadPhase3D::Clear() noexcept {
    colliders.Clear();
    grid.Clear();
}

}  // namespace Spark
