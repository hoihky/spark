#pragma once

#include "spark/engine/FrameTiming.hpp"
#include "spark/physics/PhysicsSubsystem.hpp"
#include "spark/scene/core/world/WorldView.hpp"

namespace Spark {

/** Physics stepping bound to a world + subsystem pair. */
class WorldPhysics final : public WorldView {
public:
    WorldPhysics(GameWorld& world, PhysicsSubsystem& physics) noexcept : WorldView(world), physics_(physics) {}

    void Simulate2D(const FrameTiming& timing) { physics_.Simulate2D(world_, timing); }

    void Simulate3D(const FrameTiming& timing) { physics_.Simulate3D(world_, timing); }

    void SimulateAll3D(const FrameTiming& timing) { physics_.SimulateAll3D(world_, timing); }

    void SimulateCharacters2D(const FrameTiming& timing) {
        physics_.SimulateCharacterControllers2D(world_, timing);
    }

    void SimulateCharacters3D(const FrameTiming& timing) {
        physics_.SimulateCharacterControllers3D(world_, timing);
    }

    void SimulateTriggers2D(const FrameTiming& timing) { physics_.SimulateTriggerVolumes2D(world_, timing); }

    void SimulateTriggers3D(const FrameTiming& timing) { physics_.SimulateTriggerVolumes3D(world_, timing); }

    [[nodiscard]] PhysicsSubsystem& Subsystem() noexcept { return physics_; }
    [[nodiscard]] const PhysicsSubsystem& Subsystem() const noexcept { return physics_; }

private:
    PhysicsSubsystem& physics_;
};

}  // namespace Spark
