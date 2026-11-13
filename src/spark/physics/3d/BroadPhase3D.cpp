#include "spark/physics/3d/BroadPhase3D.hpp"

#include "spark/physics/3d/colliders/ColliderBakePipeline3D.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

void BroadPhase3D::Rebuild(GameWorld& world, const float cellWorldSize) {
    ColliderBakePipeline3D::GetDefault().Rebuild(world, cellWorldSize, colliders, grid);
}

void BroadPhase3D::Clear() noexcept {
    colliders.Clear();
    grid.Clear();
}

}  // namespace Spark
