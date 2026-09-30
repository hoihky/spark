#include "spark/ecs/components/input/PlayerInputComponent.hpp"

#include "spark/core/Utility.hpp"
#include "spark/ecs/components/input/InputActionMapComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/Signal.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/input/InputAction.hpp"
#include "spark/input/InputActionSample.hpp"
#include "spark/input/InputActionTypes.hpp"

#include <cmath>
#include <cstring>

namespace Spark {

namespace {

[[nodiscard]] bool NamesEqual(const char* a, const char* b) noexcept {
    if (a == nullptr || b == nullptr) {
        return false;
    }
    return std::strcmp(a, b) == 0;
}

}  // namespace

void InputActionRuntimeState::ApplySample(const InputActionSample& sample) noexcept {
    const bool wasPressed = isPressed;
    isPressed = sample.IsButtonHeld();
    axisValue = sample.GetAxis1D();
    wasPressedThisFrame = sample.WasButtonPressedThisFrame();
    wasReleasedThisFrame = !isPressed && wasPressed;
}

void PlayerInputComponent::SyncRuntimeStates(const InputActionMapComponent& map) {
    runtimeStates.Clear();
    runtimeStates.Reserve(map.GetActions().GetSize());
    for (std::size_t i = 0; i < map.GetActions().GetSize(); ++i) {
        if (map.GetActions()[i] == nullptr) {
            continue;
        }
        InputActionRuntimeState state{};
        state.SetName(map.GetActions()[i]->GetName());
        runtimeStates.PushBack(MoveTemp(state));
    }
}

InputActionRuntimeState* PlayerInputComponent::FindRuntimeState(const char* const actionName) noexcept {
    for (std::size_t i = 0; i < runtimeStates.GetSize(); ++i) {
        if (NamesEqual(runtimeStates[i].GetName().CStr(), actionName)) {
            return &runtimeStates[i];
        }
    }
    return nullptr;
}

const InputActionRuntimeState* PlayerInputComponent::FindRuntimeState(const char* const actionName) const noexcept {
    for (std::size_t i = 0; i < runtimeStates.GetSize(); ++i) {
        if (NamesEqual(runtimeStates[i].GetName().CStr(), actionName)) {
            return &runtimeStates[i];
        }
    }
    return nullptr;
}

void PlayerInputComponent::PollAction(
        GameObject& owner,
        const InputAction& action,
        InputActionRuntimeState& state,
        IInput& input) noexcept {
    const InputActionSample sample = action.Evaluate(input);
    state.ApplySample(sample);

    if (state.WasPressedThisFrame()) {
        SignalPayload payload{};
        payload.ptr = action.GetName().CStr();
        payload.a = static_cast<std::uint64_t>(InputActionPhase::Started);
        owner.EmitSignal(SignalId::InputActionTriggered, payload, this);
    } else if (state.WasReleasedThisFrame()) {
        SignalPayload payload{};
        payload.ptr = action.GetName().CStr();
        payload.a = static_cast<std::uint64_t>(InputActionPhase::Canceled);
        owner.EmitSignal(SignalId::InputActionTriggered, payload, this);
    } else if (state.IsPressed()) {
        SignalPayload payload{};
        payload.ptr = action.GetName().CStr();
        payload.a = static_cast<std::uint64_t>(InputActionPhase::Performed);
        owner.EmitSignal(SignalId::InputActionTriggered, payload, this);
    }
}

void PlayerInputComponent::Refresh(GameObject& owner, IEngineContext& context) {
    if (actionMap == nullptr) {
        actionMap = owner.GetComponent<InputActionMapComponent>();
    }
    if (actionMap == nullptr) {
        return;
    }

    if (runtimeStates.GetSize() != actionMap->GetActions().GetSize()) {
        SyncRuntimeStates(*actionMap);
    }

    IInput& input = context.GetInput();
    for (std::size_t i = 0; i < actionMap->GetActions().GetSize(); ++i) {
        if (actionMap->GetActions()[i] == nullptr) {
            continue;
        }
        if (i >= runtimeStates.GetSize()) {
            break;
        }
        PollAction(owner, *actionMap->GetActions()[i], runtimeStates[i], input);
    }
}

void PlayerInputComponent::OnUpdate(
        const FrameTiming& /*timing*/,
        GameObject& owner,
        IEngineContext& context) {
    Refresh(owner, context);
}

bool PlayerInputComponent::IsActionPressed(const char* const actionName) const noexcept {
    const InputActionRuntimeState* state = FindRuntimeState(actionName);
    return state != nullptr && state->IsPressed();
}

bool PlayerInputComponent::WasActionPressedThisFrame(const char* const actionName) const noexcept {
    const InputActionRuntimeState* state = FindRuntimeState(actionName);
    return state != nullptr && state->WasPressedThisFrame();
}

bool PlayerInputComponent::WasActionReleasedThisFrame(const char* const actionName) const noexcept {
    const InputActionRuntimeState* state = FindRuntimeState(actionName);
    return state != nullptr && state->WasReleasedThisFrame();
}

float PlayerInputComponent::GetActionAxis1D(const char* const actionName) const noexcept {
    const InputActionRuntimeState* state = FindRuntimeState(actionName);
    return state != nullptr ? state->GetAxisValue() : 0.0F;
}

}  // namespace Spark
