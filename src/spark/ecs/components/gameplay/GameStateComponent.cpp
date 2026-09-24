#include "spark/ecs/components/gameplay/GameStateComponent.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/Signal.hpp"

namespace Spark {

bool GameStateComponent::RequestState(const GameFlowState nextState) noexcept {
    if (nextState == currentState) {
        return false;
    }
    GameObject* owner = GetOwner();
    if (owner == nullptr) {
        previousState = currentState;
        currentState = nextState;
        return true;
    }
    ApplyTransition(nextState, *owner);
    return true;
}

void GameStateComponent::PushState(const GameFlowState nextState) noexcept {
    stateStack.PushBack(currentState);
    RequestState(nextState);
}

bool GameStateComponent::PopState() noexcept {
    if (stateStack.IsEmpty()) {
        return false;
    }
    const GameFlowState restored = stateStack.GetLast();
    stateStack.PopBack();
    return RequestState(restored);
}

void GameStateComponent::ApplyTransition(const GameFlowState nextState, GameObject& owner) noexcept {
    previousState = currentState;
    currentState = nextState;

    SignalPayload payload{};
    payload.ptr = &owner;
    payload.a = static_cast<std::uint64_t>(currentState);
    payload.b = static_cast<std::uint64_t>(previousState);
    owner.EmitSignal(SignalId::GameStateChanged, payload, this);

    if (onTransition) {
        onTransition(previousState, currentState, owner);
    }
}

}  // namespace Spark
