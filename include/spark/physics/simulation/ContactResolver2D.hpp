#pragma once

#include "spark/core/Array.hpp"
#include "spark/physics/PhysicsWorld2D.hpp"
#include "spark/physics/colliders/Collider2D.hpp"
#include "spark/physics/colliders/DynamicBody2D.hpp"
#include "spark/physics/SpatialHashGrid2D.hpp"

namespace Spark {

class GameWorld;

/** Static and dynamic–dynamic contact resolution for 2D rigidbodies. */
class ContactResolver2D {
public:
    static void ResolveAllDynamicsAgainstStatics(
            GameWorld& world,
            const Array<Collider2D>& colliders,
            SpatialHashGrid2D& broadPhase);

    static void ResolveDynamicDynamicPairs(
            Array<DynamicBody2D>& bodies,
            const PhysicsWorld2DSettings& settings,
            float broadPhaseCellSize,
            SpatialHashGrid2D& dynBroad,
            Array<std::uint32_t>& pairCandidatesScratch) noexcept;
};

}  // namespace Spark
