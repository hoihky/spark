#pragma once

#include "spark/ecs/components/physics/3d/TriggerVolume3DComponent.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/physics/Collision3D.hpp"
#include "spark/physics/DynamicCollider3D.hpp"

namespace Spark {

/** World-space snapshot of a trigger volume used by overlap queries. */
struct TriggerVolume3DWorld {
    TriggerVolume3DShape shape = TriggerVolume3DShape::Box;
    CollisionAabb3 box{};
    Vector3 sphereCenter{Vector3::Zero};
    float sphereRadius = 0.5F;
    CollisionCapsule3 capsule{};
    CollisionAabb3 bounds{};
};

struct TriggerVolume3DSettings {
    bool includeCharacterControllers = true;
    bool includeDynamicRigidbodies = true;
    /** Objects with <c>CollisionComponent</c> (simple sphere bounds). */
    bool includeCollisionComponent = true;
    /** Sphere/capsule colliders without a dynamic rigidbody (kinematic / animated props). */
    bool includeColliderWithoutRigidbody = true;
};

void BuildTriggerVolume3DWorld(
        GameObject& owner,
        const TriggerVolume3DComponent& volume,
        TriggerVolume3DWorld& outWorld) noexcept;

/** Narrow-phase overlap between a trigger volume and a probe body snapshot. */
[[nodiscard]] bool TriggerVolume3DOverlapsProbe(
        const TriggerVolume3DWorld& volume,
        const DynamicCollider3DSim& probe) noexcept;

/**
 * 3D trigger-volume overlap detection. Dispatches enter/exit callbacks and
 * <c>Physics3DTriggerEnter</c> / <c>Physics3DTriggerExit</c> signals.
 */
class TriggerVolumeWorld3D {
public:
    TriggerVolumeWorld3D() = default;
    explicit TriggerVolumeWorld3D(TriggerVolume3DSettings settingsIn) noexcept : settings(settingsIn) {}

    void Simulate(GameWorld& world, const FrameTiming& timing);

    [[nodiscard]] TriggerVolume3DSettings& GetSettings() noexcept { return settings; }
    [[nodiscard]] const TriggerVolume3DSettings& GetSettings() const noexcept { return settings; }
    void SetSettings(TriggerVolume3DSettings settingsIn) noexcept { settings = settingsIn; }

private:
    TriggerVolume3DSettings settings{};
};

/** Backward-compatible free-function wrapper around <c>TriggerVolumeWorld3D::Simulate</c>. */
void SimulateTriggerVolumes3D(
        GameWorld& world,
        const FrameTiming& timing,
        const TriggerVolume3DSettings& settings = {});

}  // namespace Spark
