#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"

#include <cstdint>

namespace Spark {

class GameObject;
class GridNavAgent2DComponent;

/** How <c>GridPathFollower2DComponent</c> applies motion along the nav polyline. */
enum class GridPathFollower2DMode : std::uint8_t {
    /** Directly translates <c>TransformComponent</c> each frame. */
    Transform = 0,
    /** Sets <c>Rigidbody2DComponent</c> velocity toward the active waypoint. */
    Rigidbody2DVelocity = 1,
};

/**
 * Moves an entity along the world waypoints produced by <c>GridNavAgent2DComponent</c>.
 * Use when you do not want full <c>AiAgentComponent</c> steering (e.g. click-to-move in tilemap demos).
 *
 * Runs at priority 125 (after animation drivers, before animator playback).
 */
class GridPathFollower2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::GridPathFollower2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override { return 125; }

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    void SetEnabled(const bool value) noexcept { enabled = value; }
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    void SetNavAgent(GridNavAgent2DComponent* agent) noexcept { navAgent = agent; }
    [[nodiscard]] GridNavAgent2DComponent* GetNavAgent() const noexcept { return navAgent; }

    void SetMode(const GridPathFollower2DMode value) noexcept { mode = value; }
    [[nodiscard]] GridPathFollower2DMode GetMode() const noexcept { return mode; }

    void SetMaxSpeed(const float speed) noexcept { maxSpeed = speed; }
    [[nodiscard]] float GetMaxSpeed() const noexcept { return maxSpeed; }

    void SetArriveRadius(const float radius) noexcept { arriveRadius = radius; }
    [[nodiscard]] float GetArriveRadius() const noexcept { return arriveRadius; }

    /** Stops following and clears the linked agent path when the final waypoint is reached. */
    void SetStopOnPathEnd(const bool value) noexcept { stopOnPathEnd = value; }
    [[nodiscard]] bool GetStopOnPathEnd() const noexcept { return stopOnPathEnd; }

private:
    GridNavAgent2DComponent* navAgent = nullptr;
    GridPathFollower2DMode mode = GridPathFollower2DMode::Transform;
    float maxSpeed = 6.0F;
    float arriveRadius = 0.08F;
    bool enabled = true;
    bool stopOnPathEnd = true;
};

}  // namespace Spark
