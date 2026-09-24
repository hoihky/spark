#include "spark/physics/PhysicsSubsystem.hpp"

#include "spark/ecs/components/gameplay/PickupComponent.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

void PhysicsSubsystem::SetBroadPhaseCellSize2D(const float cellWorldSize) noexcept {
    world2D.SetBroadPhaseCellSize(cellWorldSize);
    queries2D.SetCellWorldSize(cellWorldSize);
    characterController2D.GetSettings().broadPhaseCellSize = cellWorldSize;
}

void PhysicsSubsystem::Simulate2D(GameWorld& world, const FrameTiming& timing) {
    characterController2D.GetSettings().broadPhaseCellSize = world2D.GetBroadPhaseCellSize();
    characterController2D.Prepare(world, timing);
    queries2D.SetCellWorldSize(world2D.GetBroadPhaseCellSize());
    world2D.Simulate(world, timing);
    characterController2D.Finalize(world, timing);
    triggerVolumes2D.Simulate(world, timing);
    PickupComponent::ProcessDeferredDestroys(world);
}

void PhysicsSubsystem::SimulateCharacterControllers2D(GameWorld& world, const FrameTiming& timing) {
    characterController2D.Prepare(world, timing);
    characterController2D.Finalize(world, timing);
}

void PhysicsSubsystem::SimulateTriggerVolumes2D(GameWorld& world, const FrameTiming& timing) {
    triggerVolumes2D.Simulate(world, timing);
}

void PhysicsSubsystem::Simulate3D(GameWorld& world, const FrameTiming& timing) {
    world3D.Simulate(world, timing);
}

void PhysicsSubsystem::SimulateCharacterControllers3D(GameWorld& world, const FrameTiming& timing) {
    characterController3D.Simulate(world, timing);
}

void PhysicsSubsystem::SimulateTriggerVolumes3D(GameWorld& world, const FrameTiming& timing) {
    triggerVolumes3D.Simulate(world, timing);
}

void PhysicsSubsystem::SimulateAll3D(GameWorld& world, const FrameTiming& timing) {
    world3D.Simulate(world, timing);
    characterController3D.Simulate(world, timing);
    triggerVolumes3D.Simulate(world, timing);
}

}  // namespace Spark
