#pragma once

#include "spark/physics/CharacterController3D.hpp"
#include "spark/physics/PhysicsQueries2D.hpp"
#include "spark/physics/PhysicsWorld2D.hpp"
#include "spark/physics/PhysicsWorld3D.hpp"
#include "spark/physics/TriggerVolume3D.hpp"
#include "spark/engine/FrameTiming.hpp"

namespace Spark {

class GameWorld;

/**
 * Facade over the Spark physics sub-system. Owns 2D/3D simulation worlds, query services, character controllers,
 * and trigger volumes so game code can configure once and step with a single object.
 *
 * Example:
 * @code
 * PhysicsSubsystem physics;
 * physics.GetWorld2D().GetSettings().gravityY = -30.0F;
 * physics.Simulate2D(world, timing);
 * physics.GetQueries2D().RebuildStatics(world);
 * physics.GetQueries2D().OverlapCircleStatics(px, py, 1.5F, filter, hits);
 * @endcode
 */
class PhysicsSubsystem {
public:
    /** Keeps query broad-phase cell size aligned with <c>PhysicsWorld2D</c>. */
    void SetBroadPhaseCellSize2D(float cellWorldSize) noexcept;

    void Simulate2D(GameWorld& world, const FrameTiming& timing);
    void Simulate3D(GameWorld& world, const FrameTiming& timing);
    void SimulateCharacterControllers3D(GameWorld& world, const FrameTiming& timing);
    void SimulateTriggerVolumes3D(GameWorld& world, const FrameTiming& timing);

    /** Runs 3D rigidbody simulation, character controllers, and trigger volumes in the recommended order. */
    void SimulateAll3D(GameWorld& world, const FrameTiming& timing);

    [[nodiscard]] PhysicsWorld2D& GetWorld2D() noexcept { return world2D; }
    [[nodiscard]] const PhysicsWorld2D& GetWorld2D() const noexcept { return world2D; }

    [[nodiscard]] PhysicsWorld3D& GetWorld3D() noexcept { return world3D; }
    [[nodiscard]] const PhysicsWorld3D& GetWorld3D() const noexcept { return world3D; }

    [[nodiscard]] PhysicsQueryWorld2D& GetQueries2D() noexcept { return queries2D; }
    [[nodiscard]] const PhysicsQueryWorld2D& GetQueries2D() const noexcept { return queries2D; }

    [[nodiscard]] CharacterControllerWorld3D& GetCharacterController3D() noexcept { return characterController3D; }
    [[nodiscard]] const CharacterControllerWorld3D& GetCharacterController3D() const noexcept {
        return characterController3D;
    }

    [[nodiscard]] TriggerVolumeWorld3D& GetTriggerVolumes3D() noexcept { return triggerVolumes3D; }
    [[nodiscard]] const TriggerVolumeWorld3D& GetTriggerVolumes3D() const noexcept { return triggerVolumes3D; }

private:
    PhysicsWorld2D world2D{};
    PhysicsWorld3D world3D{};
    PhysicsQueryWorld2D queries2D{};
    CharacterControllerWorld3D characterController3D{};
    TriggerVolumeWorld3D triggerVolumes3D{};
};

}  // namespace Spark
