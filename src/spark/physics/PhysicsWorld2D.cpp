#include "spark/physics/PhysicsWorld2D.hpp"

#include "spark/core/Array.hpp"
#include "spark/physics/colliders/Collider2D.hpp"
#include "spark/physics/colliders/DynamicBody2D.hpp"
#include "spark/physics/colliders/ColliderBakePipeline2D.hpp"
#include "spark/physics/simulation/ContactResolver2D.hpp"
#include "spark/physics/simulation/JointSolver2D.hpp"
#include "spark/physics/simulation/RigidbodyIntegrator2D.hpp"
#include "spark/physics/SpatialHashGrid2D.hpp"
#include "spark/scene/GameWorld.hpp"

namespace Spark {

void PhysicsWorld2D::Simulate(GameWorld& world, const FrameTiming& timing) {
    const float dt = timing.deltaTimeSeconds;
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

void SimulatePhysics2D(GameWorld& world, const FrameTiming& timing, const PhysicsWorld2DSettings& settings) {
    PhysicsWorld2D worldSim(settings);
    worldSim.Simulate(world, timing);
}

}  // namespace Spark
