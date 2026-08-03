#include "spark/physics/PhysicsSubsystem.hpp"

#include "spark/scene/GameWorld.hpp"

namespace Spark {

void PhysicsSubsystem::SetBroadPhaseCellSize2D(const float cellWorldSize) noexcept {
    world2D.SetBroadPhaseCellSize(cellWorldSize);
    queries2D.SetCellWorldSize(cellWorldSize);
}

void PhysicsSubsystem::Simulate2D(GameWorld& world, const FrameTiming& timing) {
    queries2D.SetCellWorldSize(world2D.GetBroadPhaseCellSize());
    world2D.Simulate(world, timing);
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
