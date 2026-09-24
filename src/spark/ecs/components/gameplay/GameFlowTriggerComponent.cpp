#include "spark/ecs/components/gameplay/GameFlowTriggerComponent.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/Signal.hpp"

#include <cstring>

namespace Spark {

void GameFlowTriggerComponent::SetInstigatorNameFilter(const char* const filter) noexcept {
    instigatorNameFilter = Utf8String(filter != nullptr ? filter : "");
}

GameStateComponent* GameFlowTriggerComponent::ResolveStateComponent(GameObject& owner) const noexcept {
    if (stateOwner != nullptr) {
        return stateOwner->GetComponent<GameStateComponent>();
    }
    return owner.GetComponent<GameStateComponent>();
}

void GameFlowTriggerComponent::OnSignal(GameObject& owner, const SignalId id, const SignalPayload& payload) {
    GameStateComponent* gameState = ResolveStateComponent(owner);
    if (gameState == nullptr) {
        return;
    }

    if (source == GameFlowTriggerSource::TriggerEnter && id == SignalId::Physics2DTriggerEnter) {
        const GameObject* other = static_cast<const GameObject*>(payload.ptr);
        if (other == nullptr) {
            return;
        }
        if (!instigatorNameFilter.IsEmpty()) {
            const char* name = other->GetName().CStr();
            if (name == nullptr || std::strstr(name, instigatorNameFilter.CStr()) == nullptr) {
                return;
            }
        }
        gameState->RequestState(targetState);
        return;
    }

    if (source == GameFlowTriggerSource::Died && id == SignalId::Died) {
        gameState->RequestState(targetState);
        return;
    }

    if (source == GameFlowTriggerSource::OnGameState && id == SignalId::GameStateChanged) {
        const GameFlowState newState = static_cast<GameFlowState>(payload.a);
        if (newState == watchState) {
            gameState->RequestState(targetState);
        }
    }
}

}  // namespace Spark
