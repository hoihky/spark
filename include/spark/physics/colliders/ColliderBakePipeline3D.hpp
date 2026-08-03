#pragma once

#include "spark/core/Array.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/physics/colliders/Collider3D.hpp"
#include "spark/physics/colliders/IColliderBakeStrategy3D.hpp"
#include "spark/physics/SpatialHashGrid3D.hpp"

namespace Spark {

class GameWorld;

/** 3D counterpart to <c>ColliderBakePipeline2D</c>. */
class ColliderBakePipeline3D {
public:
    void ClearStrategies() noexcept;

    void RegisterStrategy(UniquePtr<IColliderBakeStrategy3D> strategy);

    void Rebuild(
            GameWorld& world,
            float cellWorldSize,
            Array<Collider3D>& outColliders,
            SpatialHashGrid3D& outGrid);

    [[nodiscard]] static ColliderBakePipeline3D CreateDefault();
    [[nodiscard]] static ColliderBakePipeline3D& GetDefault();

    [[nodiscard]] std::size_t GetStrategyCount() const noexcept { return strategies.GetSize(); }

private:
    Array<UniquePtr<IColliderBakeStrategy3D>> strategies{};
};

}  // namespace Spark
