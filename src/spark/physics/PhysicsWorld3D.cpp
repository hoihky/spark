#include "spark/physics/PhysicsWorld3D.hpp"

#include "spark/core/Array.hpp"
#include "spark/physics/colliders/Collider3D.hpp"
#include "spark/physics/colliders/DynamicBody3D.hpp"
#include "spark/physics/colliders/ColliderBakePipeline3D.hpp"
#include "spark/physics/simulation/ContactResolver3D.hpp"
#include "spark/physics/simulation/JointSolver3D.hpp"
#include "spark/physics/simulation/RigidbodyIntegrator3D.hpp"
#include "spark/physics/SpatialHashGrid3D.hpp"
#include "spark/scene/GameWorld.hpp"

#include <algorithm>

namespace Spark {

void PhysicsWorld3D::Simulate(GameWorld& world, const FrameTiming& timing) {
    const float dt = timing.deltaTimeSeconds;
    if (dt <= 0.0F) {
        return;
    }

    const int substeps = std::max(1, settings.substeps);
    const float h = dt / static_cast<float>(substeps);

    Array<Collider3D> colliders;
    SpatialHashGrid3D broadPhase;
    ColliderBakePipeline3D::GetDefault().Rebuild(world, broadPhaseCellSize, colliders, broadPhase);

    Array<std::uint32_t> queryScratch;
    Array<DynamicBody3D> bodies;
    CollectDynamicBodies3D(world, bodies);

    for (int s = 0; s < substeps; ++s) {
        RigidbodyIntegrator3D::IntegrateSubstep(bodies, h, settings, colliders, broadPhase, queryScratch);
        ContactResolver3D::ApplyStaticNormalImpulses(bodies, colliders, broadPhase, queryScratch, h, settings);
        ContactResolver3D::ResolvePenetrations(bodies, colliders, broadPhase, queryScratch, settings);
        ContactResolver3D::ResolveDynamicPairVelocities(bodies, h, settings);
        JointSolver3D::SolveSubstep(world, h, settings);
    }
}

void SimulatePhysics3D(GameWorld& world, const FrameTiming& timing, const PhysicsWorld3DSettings& settings) {
    PhysicsWorld3D worldSim(settings);
    worldSim.Simulate(world, timing);
}

}  // namespace Spark
