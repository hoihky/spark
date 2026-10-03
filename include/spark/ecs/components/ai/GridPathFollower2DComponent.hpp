#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"

#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;
class GridNavAgent2DComponent;

/** How <c>GridPathFollower2DComponent</c> applies motion along the nav polyline. */
enum class GridPathFollower2DMode : std::uint8_t {
    /** Directly translates <c>TransformComponent</c> each frame. */
    Transform = 0,
    /** Sets <c>Rigidbody2DComponent</c> velocity toward the active waypoint. */
    Rigidbody2DVelocity = 1,
    /** Uses <c>SteeringPathFollowing2D</c> + optional obstacle avoidance before applying velocity. */
    Steering2D = 2,
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

    SPARK_SCRIPT_BIND(set_enabled)
    void SetEnabled(const bool value) noexcept { enabled = value; }
    SPARK_SCRIPT_BIND(is_enabled)
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    SPARK_SCRIPT_BIND(set_nav_agent)
    void SetNavAgent(GridNavAgent2DComponent* agent) noexcept { navAgent = agent; }
    SPARK_SCRIPT_BIND(get_nav_agent)
    [[nodiscard]] GridNavAgent2DComponent* GetNavAgent() const noexcept { return navAgent; }

    SPARK_SCRIPT_BIND(set_mode)
    void SetMode(const GridPathFollower2DMode value) noexcept { mode = value; }
    SPARK_SCRIPT_BIND(get_mode)
    [[nodiscard]] GridPathFollower2DMode GetMode() const noexcept { return mode; }

    SPARK_SCRIPT_BIND(set_max_speed)
    void SetMaxSpeed(const float speed) noexcept { maxSpeed = speed; }
    SPARK_SCRIPT_BIND(get_max_speed)
    [[nodiscard]] float GetMaxSpeed() const noexcept { return maxSpeed; }

    SPARK_SCRIPT_BIND(set_arrive_radius)
    void SetArriveRadius(const float radius) noexcept { arriveRadius = radius; }
    SPARK_SCRIPT_BIND(get_arrive_radius)
    [[nodiscard]] float GetArriveRadius() const noexcept { return arriveRadius; }

    /** Stops following and clears the linked agent path when the final waypoint is reached. */
    SPARK_SCRIPT_BIND(set_stop_on_path_end)
    void SetStopOnPathEnd(const bool value) noexcept { stopOnPathEnd = value; }
    SPARK_SCRIPT_BIND(get_stop_on_path_end)
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
