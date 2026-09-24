#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Function.hpp"
#include "spark/ecs/GameComponent.hpp"

#include <cstdint>

namespace Spark {

class GameObject;

/** High-level game flow states for 2D/3D demos and prototypes. */
enum class GameFlowState : std::uint8_t {
    Intro = 0,
    Playing = 1,
    Paused = 2,
    Victory = 3,
    Defeat = 4,
};

/**
 * Owns the active game-flow state (State pattern). Emits <c>SignalId::GameStateChanged</c>
 * to sibling components when transitions occur.
 */
class GameStateComponent final : public GameComponent {
public:
    using TransitionCallback = Function<void(GameFlowState previous, GameFlowState current, GameObject& owner)>;

    static constexpr ComponentKind TypeKind = ComponentKind::GameState;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit GameStateComponent(GameFlowState initialState = GameFlowState::Playing) noexcept
            : currentState(initialState), previousState(initialState) {}

    [[nodiscard]] GameFlowState GetState() const noexcept { return currentState; }
    [[nodiscard]] GameFlowState GetPreviousState() const noexcept { return previousState; }

    [[nodiscard]] bool IsState(const GameFlowState state) const noexcept { return currentState == state; }

    /** Changes state when different; returns true when a transition occurred. */
    bool RequestState(GameFlowState nextState) noexcept;

    /** Pushes current state and enters <c>nextState</c> (e.g. pause menu). */
    void PushState(GameFlowState nextState) noexcept;

    /** Restores the last pushed state when the stack is non-empty. */
    bool PopState() noexcept;

    void SetOnTransition(TransitionCallback callback) { onTransition = MoveTemp(callback); }

private:
    void ApplyTransition(GameFlowState nextState, GameObject& owner) noexcept;

    GameFlowState currentState = GameFlowState::Playing;
    GameFlowState previousState = GameFlowState::Playing;
    Array<GameFlowState> stateStack{};
    TransitionCallback onTransition{};
};

}  // namespace Spark
