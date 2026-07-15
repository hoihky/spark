#pragma once

#include "spark/ai/fsm/IFsmState.hpp"
#include "spark/core/Array.hpp"
#include "spark/memory/UniquePtr.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

class AiBlackboard;

/** One row in a transition table (data-driven FSM). */
struct FsmTransition {
    std::uint32_t fromState = 0;
    std::uint32_t eventId = 0;
    std::uint32_t toState = 0;
};

/**
 * Finite state machine with explicit transition table and polymorphic states (Liskov: states via IFsmState).
 * Dependency inversion: machine depends on IFsmState, not concrete gameplay types.
 */
class FsmStateMachine {
public:
    FsmStateMachine() = default;
    FsmStateMachine(const FsmStateMachine&) = delete;
    FsmStateMachine& operator=(const FsmStateMachine&) = delete;
    FsmStateMachine(FsmStateMachine&&) noexcept = default;
    FsmStateMachine& operator=(FsmStateMachine&&) noexcept = default;

    /** States are indexed 0..N-1; index is the state id used in transitions. */
    void AddState(UniquePtr<IFsmState> state);
    void AddTransition(const FsmTransition& rule);

    void SetInitialState(const std::uint32_t stateId) noexcept;
    [[nodiscard]] std::uint32_t GetCurrentState() const noexcept { return current; }

    /** Returns true if a transition fired. */
    bool SendEvent(const std::uint32_t eventId, AiBlackboard& board);

    void Tick(const FrameTiming& timing, AiBlackboard& board);

private:
    void EnterState_(const std::uint32_t next, AiBlackboard& board);
    [[nodiscard]] IFsmState* StateAt_(std::uint32_t id) const noexcept;

    Array<UniquePtr<IFsmState>> states;
    Array<FsmTransition> transitions;
    std::uint32_t current = 0;
    std::uint32_t initial = 0;
    bool booted = false;
};

}  // namespace Spark
