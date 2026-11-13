#include "spark/physics/2d/PhysicsWorld2D.hpp"

#include "spark/core/Array.hpp"
#include "spark/physics/2d/colliders/Collider2D.hpp"
#include "spark/physics/2d/colliders/DynamicBody2D.hpp"
#include "spark/physics/2d/colliders/ColliderBakePipeline2D.hpp"
#include "spark/physics/2d/simulation/ContactResolver2D.hpp"
#include "spark/physics/2d/simulation/JointSolver2D.hpp"
#include "spark/physics/2d/simulation/RigidbodyIntegrator2D.hpp"
#include "spark/physics/2d/SpatialHashGrid2D.hpp"
#include "spark/scene/core/GameWorld.hpp"

#include <algorithm>

namespace Spark {

namespace {

constexpr float kMaxPhysicsStepSeconds2D = 1.0F / 30.0F;

}  // namespace

void PhysicsWorld2D::Simulate(GameWorld& world, const FrameTiming& timing) {
    const float dt = std::min(timing.deltaTimeSeconds, kMaxPhysicsStepSeconds2D);
    if (dt <= 0.0F) {
        return;
    }

    Array<Collider2D> colliders;
    SpatialHashGrid2D broadPhase;
    ColliderBakePipeline2D::GetDefault().Rebuild(world, broadPhaseCellSize, colliders, broadPhase);

    RigidbodyIntegrator2D::Integrate(world, dt, settings);
    ContactResolver2D::ResolveAllDynamicsAgainstStatics(world, colliders, broadPhase);

    Array<DynamicBody2D> dynamics;
    SpatialHashGrid2D dynBroad;
    Array<std::uint32_t> dynPairScratch;
    CollectDynamicBodies2D(world, dynamics);
    ContactResolver2D::ResolveDynamicDynamicPairs(dynamics, settings, broadPhaseCellSize, dynBroad, dynPairScratch);

    JointSolver2D::Solve(world, settings);
}

}  // namespace Spark
