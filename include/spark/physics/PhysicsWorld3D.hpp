#pragma once

#include "spark/engine/FrameTiming.hpp"

namespace Spark {

class GameWorld;

/** Vertical gravity (+Y up); typical -24. */
struct PhysicsWorld3DSettings {
    float gravityY = -24.0F;
    float maxFallSpeed = 80.0F;
    int resolveIterations = 8;
    /** Fixed substeps per frame for stability (≥1). Each substep uses <c>deltaTime / substeps</c>. */
    int substeps = 1;
    /** Soft position iterations for <c>DistanceJoint3DComponent</c> constraints per substep. */
    int jointIterations = 0;
    /**
     * Baumgarte-style normal separation velocity added after contact impulses on the first position iteration
     * (reduces residual penetration from repeated projections). Set to 0 to disable.
     */
    float baumgarteContactBias = 0.14F;
    float baumgarteMaxSeparationVelocity = 8.0F;
    float contactPenetrationSlop = 0.0015F;
    /**
     * Binary-search steps along the proposed translation vs statics (0 = disabled).
     * Uses a conservative inflated-AABB query along the motion segment to reduce tunneling.
     */
    int sweptStaticCcdBinaryIterations = 10;
    /**
     * Kinetic Coulomb friction applies only for contacts with meaningful normal approach (m/s). Static contacts use
     * pre-restitution <c>v·n</c> (negative into the surface); sphere–sphere uses positive closing speed along <c>n</c>.
     * Stops repeated position iterations from treating resting/sliding overlap as impacts (avoids tangential "glue").
     */
    float frictionImpactNormalSpeed = 0.48F;
};

/**
 * 3D physics simulation world. Integrates dynamic sphere/capsule rigidbodies against static box/capsule geometry,
 * with optional swept CCD, Baumgarte contact bias, and joint constraints.
 */
class PhysicsWorld3D {
public:
    static constexpr float DefaultBroadPhaseCellSize = 2.0F;

    PhysicsWorld3D() = default;
    explicit PhysicsWorld3D(PhysicsWorld3DSettings settingsIn) noexcept : settings(settingsIn) {}

    void Simulate(GameWorld& world, const FrameTiming& timing);

    [[nodiscard]] PhysicsWorld3DSettings& GetSettings() noexcept { return settings; }
    [[nodiscard]] const PhysicsWorld3DSettings& GetSettings() const noexcept { return settings; }
    void SetSettings(PhysicsWorld3DSettings settingsIn) noexcept { settings = settingsIn; }

    void SetBroadPhaseCellSize(float cellWorldSize) noexcept { broadPhaseCellSize = cellWorldSize; }
    [[nodiscard]] float GetBroadPhaseCellSize() const noexcept { return broadPhaseCellSize; }

private:
    PhysicsWorld3DSettings settings{};
    float broadPhaseCellSize = DefaultBroadPhaseCellSize;
};

/** @deprecated Prefer <c>PhysicsSubsystem::Simulate3D</c> or <c>PhysicsWorld3D::Simulate</c>. */
[[deprecated("Use PhysicsSubsystem::Simulate3D or PhysicsWorld3D::Simulate")]]
void SimulatePhysics3D(GameWorld& world, const FrameTiming& timing, const PhysicsWorld3DSettings& settings = {});

}  // namespace Spark
