#pragma once

#include "spark/engine/FrameTiming.hpp"

namespace Spark {

class GameWorld;

/** Vertical gravity acceleration (world +Y is up); typical value -32. */
struct PhysicsWorld2DSettings {
    float gravityY = -32.0F;
    float maxFallSpeed = 46.0F;
    /**
     * When true, overlapping **non-trigger** dynamic pairs receive a single lightweight positional separation
     * after static resolution (circle–circle, box–box, box–circle). Triggers never separate; they only emit
     * <c>Physics2DTriggerOverlap</c>. A second static pass is not run (fast-moving bodies may re-penetrate statics).
     */
    bool resolveDynamicVsDynamic = false;
    /** Soft position iterations for 2D distance/hinge joints per physics step. */
    int jointIterations = 4;
};

/**
 * 2D physics simulation world. Integrates dynamic rigidbodies against static geometry using a spatial-hash
 * broad-phase and narrow-phase contact resolution.
 *
 * Prefer holding a <c>PhysicsWorld2D</c> instance (or using <c>PhysicsSubsystem</c>) over calling
 * <c>SimulatePhysics2D</c> directly — settings persist across frames and the API is easier to extend.
 */
class PhysicsWorld2D {
public:
    static constexpr float DefaultBroadPhaseCellSize = 4.0F;

    PhysicsWorld2D() = default;
    explicit PhysicsWorld2D(PhysicsWorld2DSettings settingsIn) noexcept : settings(settingsIn) {}

    void Simulate(GameWorld& world, const FrameTiming& timing);

    [[nodiscard]] PhysicsWorld2DSettings& GetSettings() noexcept { return settings; }
    [[nodiscard]] const PhysicsWorld2DSettings& GetSettings() const noexcept { return settings; }
    void SetSettings(PhysicsWorld2DSettings settingsIn) noexcept { settings = settingsIn; }

    void SetBroadPhaseCellSize(float cellWorldSize) noexcept { broadPhaseCellSize = cellWorldSize; }
    [[nodiscard]] float GetBroadPhaseCellSize() const noexcept { return broadPhaseCellSize; }

private:
    PhysicsWorld2DSettings settings{};
    float broadPhaseCellSize = DefaultBroadPhaseCellSize;
};

/**
 * Integrates dynamic rigidbodies (Rigidbody2D Dynamic + Transform + BoxCollider2D and/or CircleCollider2D).
 *
 * @deprecated Prefer <c>PhysicsSubsystem::Simulate2D</c> or <c>PhysicsWorld2D::Simulate</c> so settings
 * persist across frames.
 */
[[deprecated("Use PhysicsSubsystem::Simulate2D or PhysicsWorld2D::Simulate")]]
void SimulatePhysics2D(GameWorld& world, const FrameTiming& timing, const PhysicsWorld2DSettings& settings = {});

}  // namespace Spark
