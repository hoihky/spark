#pragma once

#include "spark/core/Array.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/physics/colliders/Collider2D.hpp"
#include "spark/physics/colliders/IColliderBakeStrategy2D.hpp"
#include "spark/physics/SpatialHashGrid2D.hpp"

namespace Spark {

class GameWorld;

/**
 * Registry of <c>IColliderBakeStrategy2D</c> implementations. Iterates active objects and
 * dispatches to every strategy whose <c>Contributes</c> check passes (Strategy + Registry).
 */
class ColliderBakePipeline2D {
public:
    void ClearStrategies() noexcept;

    /** Takes ownership of <c>strategy</c> (append order defines bake order). */
    void RegisterStrategy(UniquePtr<IColliderBakeStrategy2D> strategy);

    void Rebuild(
            GameWorld& world,
            float cellWorldSize,
            Array<Collider2D>& outColliders,
            SpatialHashGrid2D& outGrid);

    /** Pipeline preloaded with tilemap, polygon, box, and circle strategies. */
    [[nodiscard]] static ColliderBakePipeline2D CreateDefault();

    /** Process-wide default pipeline (mutable so demos/tests can register extensions). */
    [[nodiscard]] static ColliderBakePipeline2D& GetDefault();

    [[nodiscard]] std::size_t GetStrategyCount() const noexcept { return strategies.GetSize(); }

private:
    Array<UniquePtr<IColliderBakeStrategy2D>> strategies{};
};

}  // namespace Spark
