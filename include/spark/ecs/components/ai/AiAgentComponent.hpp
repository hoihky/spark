#pragma once

#include "spark/ai/AiBlackboard.hpp"
#include "spark/ai/fsm/FiniteStateMachine.hpp"
#include "spark/ai/fuzzy/FuzzyLogic.hpp"
#include "spark/ai/goap/GoapTypes.hpp"
#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/core/Utility.hpp"

#include <cstddef>
#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;
class IEngineContext;

enum class AiSteeringPlane : std::uint8_t {
    /** Steering Vector2.x → world X, Vector2.y → world Z (Y stays unchanged when driven from AI). */
    XzWorld = 0,
    /** Steering maps directly onto <c>Rigidbody2DComponent</c> velocity X/Y. */
    XyRigidbody2D = 1,
};

/**
 * ECS hook for the AI subsystem: blackboard + optional FSM, GOAP library, path polyline, fuzzy module, and motion
 * parameters. Enable flags keep unused modules cheap.
 */
class AiAgentComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::AiAgent;

    AiAgentComponent() noexcept = default;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    SPARK_SCRIPT_BIND(get_blackboard)
    [[nodiscard]] AiBlackboard& GetBlackboard() noexcept { return blackboard; }
    SPARK_SCRIPT_BIND(get_blackboard)
    [[nodiscard]] const AiBlackboard& GetBlackboard() const noexcept { return blackboard; }

    SPARK_SCRIPT_BIND(is_enabled)
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    SPARK_SCRIPT_BIND(set_enabled)
    void SetEnabled(const bool e) noexcept { enabled = e; }

    SPARK_SCRIPT_BIND(is_fsm_enabled)
    [[nodiscard]] bool IsFsmEnabled() const noexcept { return fsmEnabled; }
    SPARK_SCRIPT_BIND(set_fsm_enabled)
    void SetFsmEnabled(const bool e) noexcept { fsmEnabled = e; }

    SPARK_SCRIPT_BIND(try_get_fsm)
    [[nodiscard]] FsmStateMachine* TryGetFsm() noexcept { return fsm.Get(); }
    SPARK_SCRIPT_BIND(try_get_fsm)
    [[nodiscard]] const FsmStateMachine* TryGetFsm() const noexcept { return fsm.Get(); }
    void SetFsm(UniquePtr<FsmStateMachine> machine) noexcept { fsm = MoveTemp(machine); }

    SPARK_SCRIPT_BIND(get_max_speed)
    [[nodiscard]] float GetMaxSpeed() const noexcept { return maxSpeed; }
    SPARK_SCRIPT_BIND(set_max_speed)
    void SetMaxSpeed(const float s) noexcept { maxSpeed = s; }

    SPARK_SCRIPT_BIND(get_steering_plane)
    [[nodiscard]] AiSteeringPlane GetSteeringPlane() const noexcept { return plane; }
    SPARK_SCRIPT_BIND(set_steering_plane)
    void SetSteeringPlane(const AiSteeringPlane p) noexcept { plane = p; }

    SPARK_SCRIPT_BIND(is_goap_enabled)
    [[nodiscard]] bool IsGoapEnabled() const noexcept { return goapEnabled; }
    SPARK_SCRIPT_BIND(set_goap_enabled)
    void SetGoapEnabled(const bool e) noexcept { goapEnabled = e; }
    SPARK_SCRIPT_BIND(get_goap_world_bits)
    [[nodiscard]] std::uint64_t GetGoapWorldBits() const noexcept { return goapWorldBits; }
    SPARK_SCRIPT_BIND(set_goap_world_bits)
    void SetGoapWorldBits(const std::uint64_t w) noexcept { goapWorldBits = w; }
    SPARK_SCRIPT_BIND(get_goap_goal_mask)
    [[nodiscard]] std::uint64_t GetGoapGoalMask() const noexcept { return goapGoalMask; }
    SPARK_SCRIPT_BIND(get_goap_goal_value)
    [[nodiscard]] std::uint64_t GetGoapGoalValue() const noexcept { return goapGoalValue; }
    void SetGoapGoal(const std::uint64_t mask, const std::uint64_t value) noexcept {
        goapGoalMask = mask;
        goapGoalValue = value;
    }
    [[nodiscard]] Array<GoapActionSpec>& GetGoapActions() noexcept { return goapActions; }
    [[nodiscard]] const Array<GoapActionSpec>& GetGoapActions() const noexcept { return goapActions; }
    [[nodiscard]] Array<std::uint32_t>& GetGoapPlan() noexcept { return goapPlan; }

    [[nodiscard]] Array<Vector2>& GetPathWorldPolylineXZ() noexcept { return pathWorldXZ; }
    [[nodiscard]] const Array<Vector2>& GetPathWorldPolylineXZ() const noexcept { return pathWorldXZ; }
    SPARK_SCRIPT_BIND(get_path_index)
    [[nodiscard]] int GetPathIndex() const noexcept { return pathIndex; }
    SPARK_SCRIPT_BIND(set_path_index)
    void SetPathIndex(const int i) noexcept { pathIndex = i; }
    void ClearPath() noexcept {
        pathWorldXZ.Clear();
        pathIndex = 0;
    }

    SPARK_SCRIPT_BIND(is_fuzzy_enabled)
    [[nodiscard]] bool IsFuzzyEnabled() const noexcept { return fuzzyEnabled; }
    SPARK_SCRIPT_BIND(set_fuzzy_enabled)
    void SetFuzzyEnabled(const bool e) noexcept { fuzzyEnabled = e; }
    void SetFuzzyModule(UniquePtr<FuzzyAdvisoryModule> module) noexcept { fuzzyModule = MoveTemp(module); }
    SPARK_SCRIPT_BIND(try_get_fuzzy_module)
    [[nodiscard]] FuzzyAdvisoryModule* TryGetFuzzyModule() noexcept { return fuzzyModule.Get(); }

    /** Called by <c>SimulateGameAi</c> once per frame when the agent is enabled. */
    void SubsystemTick(const FrameTiming& timing, GameObject& owner, IEngineContext& context);

private:
    AiBlackboard blackboard{};
    bool enabled = true;
    bool fsmEnabled = false;
    UniquePtr<FsmStateMachine> fsm{};
    float maxSpeed = 4.0F;
    AiSteeringPlane plane = AiSteeringPlane::XzWorld;

    bool goapEnabled = false;
    std::uint64_t goapWorldBits = 0;
    std::uint64_t goapGoalMask = 0;
    std::uint64_t goapGoalValue = 0;
    Array<GoapActionSpec> goapActions{};
    Array<std::uint32_t> goapPlan{};

    Array<Vector2> pathWorldXZ{};
    int pathIndex = 0;

    bool fuzzyEnabled = false;
    UniquePtr<FuzzyAdvisoryModule> fuzzyModule{};
};

}  // namespace Spark
