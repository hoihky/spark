#include "spark/physics/3d/PhysicsWorld3D.hpp"

#include "spark/core/Array.hpp"
#include "spark/physics/3d/colliders/Collider3D.hpp"
#include "spark/physics/3d/colliders/DynamicBody3D.hpp"
#include "spark/physics/3d/colliders/ColliderBakePipeline3D.hpp"
#include "spark/physics/3d/simulation/ContactResolver3D.hpp"
#include "spark/physics/3d/simulation/JointSolver3D.hpp"
#include "spark/physics/3d/PhysicsBodyHandoff3D.hpp"
#include "spark/physics/3d/simulation/RigidbodyIntegrator3D.hpp"
#include "spark/physics/3d/SpatialHashGrid3D.hpp"
#include "spark/scene/core/GameWorld.hpp"

#include <algorithm>

namespace Spark {

void PhysicsWorld3D::ResetFixedStepAccumulator() noexcept {
    fixedStepClock.ResetAccumulator();
}

void PhysicsWorld3D::SimulateStep(GameWorld& world, const float stepDeltaSeconds) {
    if (stepDeltaSeconds <= 0.0F) {
        return;
    }

    PhysicsBodyHandoff3D::SynchronizeAttachedBodies(world);
    if (!simulationDomain.ShouldAdvanceLiveSimulation()) {
        return;
    }

    const int substeps = std::max(1, settings.substeps);
    const float h = stepDeltaSeconds / static_cast<float>(substeps);

    Array<Collider3D> colliders;
    SpatialHashGrid3D broadPhase;
    ColliderBakePipeline3D::GetDefault().Rebuild(world, broadPhaseCellSize, colliders, broadPhase);

    Array<std::uint32_t> queryScratch;
    Array<DynamicBody3D> bodies;
    CollectDynamicBodies3D(world, bodies);

    for (int s = 0; s < substeps; ++s) {
        contactBroadcaster.BeginSubstep();
        RigidbodyIntegrator3D::IntegrateSubstep(bodies, h, settings, colliders, broadPhase, queryScratch);
        ContactResolver3D::ApplyStaticNormalImpulses(
                bodies, colliders, broadPhase, queryScratch, h, settings, &contactBroadcaster);
        ContactResolver3D::ResolvePenetrations(bodies, colliders, broadPhase, queryScratch, settings);
        ContactResolver3D::ResolveDynamicPairVelocities(bodies, h, settings, &contactBroadcaster);
        JointSolver3D::SolveSubstep(world, h, settings);
        contactBroadcaster.Flush(world);
    }

    simulationDomain.OnLiveStepFinished(world);
}

void PhysicsWorld3D::Simulate(GameWorld& world, const FrameTiming& timing) {
    const float frameDt = timing.deltaTimeSeconds;
    if (frameDt <= 0.0F) {
        return;
    }

    fixedStepClock.SetFixedTimestepSeconds(settings.fixedSimulationTimestep);
    fixedStepClock.SetMaxStepsPerFrame(settings.maxFixedSimulationStepsPerFrame);

    const int steps = fixedStepClock.BeginFrame(frameDt);
    for (int i = 0; i < steps; ++i) {
        SimulateStep(world, fixedStepClock.GetStepDeltaSeconds());
    }
}

}  // namespace Spark
