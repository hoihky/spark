#include "spark/ecs/components/script/ScriptSignalListenerComponent.hpp"

#include "spark/ecs/GameObject.hpp"

namespace Spark {

void ScriptSignalListenerComponent::SetScriptCallback(const ScriptSignalCallback callback, void* userData) noexcept {
    scriptCallback = callback;
    scriptUserData = userData;
}

void ScriptSignalListenerComponent::OnDetach(GameObject& owner) {
    (void)owner;
    scriptCallback = nullptr;
    scriptUserData = nullptr;
}

void ScriptSignalListenerComponent::OnSignal(
        GameObject& owner,
        const SignalId id,
        const SignalPayload& payload) {
    if (scriptCallback == nullptr) {
        return;
    }
    if (listenSignal != SignalId::None && listenSignal != id) {
        return;
    }
    scriptCallback(scriptUserData, &owner, id, payload);
}

}  // namespace Spark
