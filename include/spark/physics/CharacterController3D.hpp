#pragma once

#include "spark/engine/FrameTiming.hpp"

namespace Spark {

class GameWorld;

struct CharacterController3DSettings {
    float gravityY = -24.0F;
    float maxFallSpeed = 80.0F;
    /** Slide-and-sweep iterations per substep when resolving translation vs static boxes. */
    int slideIterations = 3;
    /** Binary-search steps when shortening a translation segment before overlap. */
    int sweepBinaryIterations = 8;
    float broadPhaseCellSize = 2.0F;
};

/**
 * 3D kinematic character-controller motor. Integrates every active <c>CharacterController3DComponent</c>
 * against static box/capsule geometry (same broad-phase as <c>PhysicsWorld3D</c>).
 */
class CharacterControllerWorld3D {
public:
    CharacterControllerWorld3D() = default;
    explicit CharacterControllerWorld3D(CharacterController3DSettings settingsIn) noexcept : settings(settingsIn) {}

    void Simulate(GameWorld& world, const FrameTiming& timing);

    [[nodiscard]] CharacterController3DSettings& GetSettings() noexcept { return settings; }
    [[nodiscard]] const CharacterController3DSettings& GetSettings() const noexcept { return settings; }
    void SetSettings(CharacterController3DSettings settingsIn) noexcept { settings = settingsIn; }

private:
    CharacterController3DSettings settings{};
};

/** Backward-compatible free-function wrapper around <c>CharacterControllerWorld3D::Simulate</c>. */
void SimulateCharacterControllers3D(
        GameWorld& world,
        const FrameTiming& timing,
        const CharacterController3DSettings& settings = {});

}  // namespace Spark
