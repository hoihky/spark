#pragma once

#include "spark/engine/FrameTiming.hpp"
#include "spark/physics/PhysicsSubsystem.hpp"
#include "spark/scene/core/world/WorldView.hpp"

namespace Spark {

/** Physics stepping bound to a world + subsystem pair. */
class WorldPhysics final : public WorldView {
public:
    WorldPhysics(GameWorld& world, PhysicsSubsystem& physics) noexcept
        : WorldView(world)
        , physicsSubsystem(physics) {}

    void Simulate2D(const FrameTiming& timing) { physicsSubsystem.Simulate2D(hostWorld, timing); }

    void Simulate3D(const FrameTiming& timing) { physicsSubsystem.Simulate3D(hostWorld, timing); }

    void SimulateAll3D(const FrameTiming& timing) { physicsSubsystem.SimulateAll3D(hostWorld, timing); }

    void SimulateCharacters2D(const FrameTiming& timing) {
        physicsSubsystem.SimulateCharacterControllers2D(hostWorld, timing);
    }

    void SimulateCharacters3D(const FrameTiming& timing) {
        physicsSubsystem.SimulateCharacterControllers3D(hostWorld, timing);
    }

    void SimulateTriggers2D(const FrameTiming& timing) {
        physicsSubsystem.SimulateTriggerVolumes2D(hostWorld, timing);
    }

    void SimulateTriggers3D(const FrameTiming& timing) {
        physicsSubsystem.SimulateTriggerVolumes3D(hostWorld, timing);
    }

    [[nodiscard]] PhysicsSubsystem& Subsystem() noexcept { return physicsSubsystem; }
    [[nodiscard]] const PhysicsSubsystem& Subsystem() const noexcept { return physicsSubsystem; }

private:
    PhysicsSubsystem& physicsSubsystem;
};

}  // namespace Spark