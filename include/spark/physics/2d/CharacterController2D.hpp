#pragma once

#include "spark/engine/FrameTiming.hpp"

namespace Spark {

class Collider2D;
class GameObject;
class Rigidbody2DComponent;
class GameWorld;

struct CharacterController2DSettings {
    float broadPhaseCellSize = 4.0F;
};

/**
 * Prepare/finalize pass for every <c>CharacterController2DComponent</c> paired with
 * <c>Rigidbody2DComponent</c>. Prepare runs before <c>PhysicsWorld2D::Simulate</c>;
 * finalize runs after contact resolution (ground snap + slope-aware grounded probe).
 */
class CharacterControllerWorld2D {
public:
    CharacterControllerWorld2D() = default;
    explicit CharacterControllerWorld2D(CharacterController2DSettings settingsIn) noexcept
            : settings(settingsIn) {}

    void Prepare(GameWorld& world, const FrameTiming& timing);
    void Finalize(GameWorld& world, const FrameTiming& timing);

    [[nodiscard]] CharacterController2DSettings& GetSettings() noexcept { return settings; }
    [[nodiscard]] const CharacterController2DSettings& GetSettings() const noexcept { return settings; }
    void SetSettings(CharacterController2DSettings settingsIn) noexcept { settings = settingsIn; }

private:
    CharacterController2DSettings settings{};
};

void SimulateCharacterControllers2D(
        GameWorld& world,
        const FrameTiming& timing,
        const CharacterController2DSettings& settings = {});

/** Returns false when a one-way platform should not block the dynamic body this step. */
[[nodiscard]] bool ShouldResolveCharacterAgainstStatic2D(
        GameObject& dynamicObject,
        Rigidbody2DComponent& rigidbody,
        const Collider2D& staticCollider,
        float dynamicFeetY) noexcept;

}  // namespace Spark
