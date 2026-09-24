#include "spark/ecs/components/input/PlayerInputComponent.hpp"

#include "spark/core/Utility.hpp"
#include "spark/ecs/components/input/InputActionMapComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/Signal.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/IInput.hpp"

#include <algorithm>
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

[[nodiscard]] bool IsKeyPressed(const IInput& input, const int keyCode) noexcept {
    return keyCode >= 0 && input.IsKeyDown(keyCode);
}

[[nodiscard]] bool WasKeyPressedThisFrame(const IInput& input, const int keyCode) noexcept {
    return keyCode >= 0 && input.IsKeyPressedThisFrame(keyCode);
}

}  // namespace

void PlayerInputComponent::SyncRuntimeStates(const InputActionMapComponent& map) {
    runtimeStates.Clear();
    runtimeStates.Reserve(map.GetActions().GetSize());
    for (std::size_t i = 0; i < map.GetActions().GetSize(); ++i) {
        InputActionRuntimeState state{};
        state.name = map.GetActions()[i].name;
        runtimeStates.PushBack(MoveTemp(state));
    }
}

InputActionRuntimeState* PlayerInputComponent::FindRuntimeState(const char* const actionName) noexcept {
    for (std::size_t i = 0; i < runtimeStates.GetSize(); ++i) {
        if (NamesEqual(runtimeStates[i].name.CStr(), actionName)) {
            return &runtimeStates[i];
        }
    }
    return nullptr;
}

const InputActionRuntimeState* PlayerInputComponent::FindRuntimeState(const char* const actionName) const noexcept {
    for (std::size_t i = 0; i < runtimeStates.GetSize(); ++i) {
        if (NamesEqual(runtimeStates[i].name.CStr(), actionName)) {
            return &runtimeStates[i];
        }
    }
    return nullptr;
}

void PlayerInputComponent::PollAction(
        GameObject& owner,
        const InputActionDefinition& definition,
        InputActionRuntimeState& state,
        IInput& input) noexcept {
    const bool wasPressed = state.isPressed;
    bool isPressed = false;
    float axis = 0.0F;

    if (definition.type == InputActionType::Button) {
        isPressed = IsKeyPressed(input, definition.primaryKey) || IsKeyPressed(input, definition.secondaryKey);
        axis = isPressed ? 1.0F : 0.0F;
    } else {
        const bool neg = IsKeyPressed(input, definition.negativeKey) ||
                IsKeyPressed(input, definition.secondaryNegativeKey);
        const bool pos = IsKeyPressed(input, definition.positiveKey) ||
                IsKeyPressed(input, definition.secondaryPositiveKey);
        if (neg) {
            axis -= 1.0F;
        }
        if (pos) {
            axis += 1.0F;
        }
        axis = std::clamp(axis, -1.0F, 1.0F);
        isPressed = std::fabs(axis) > 0.001F;
    }

    state.wasPressedThisFrame = isPressed && !wasPressed;
    state.wasReleasedThisFrame = !isPressed && wasPressed;
    state.isPressed = isPressed;
    state.axisValue = axis;

    if (state.wasPressedThisFrame) {
        SignalPayload payload{};
        payload.ptr = definition.name.CStr();
        payload.a = static_cast<std::uint64_t>(InputActionPhase::Started);
        owner.EmitSignal(SignalId::InputActionTriggered, payload, this);
    } else if (state.wasReleasedThisFrame) {
        SignalPayload payload{};
        payload.ptr = definition.name.CStr();
        payload.a = static_cast<std::uint64_t>(InputActionPhase::Canceled);
        owner.EmitSignal(SignalId::InputActionTriggered, payload, this);
    } else if (isPressed) {
        SignalPayload payload{};
        payload.ptr = definition.name.CStr();
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
        PollAction(owner, actionMap->GetActions()[i], runtimeStates[i], input);
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
    return state != nullptr && state->isPressed;
}

bool PlayerInputComponent::WasActionPressedThisFrame(const char* const actionName) const noexcept {
    const InputActionRuntimeState* state = FindRuntimeState(actionName);
    return state != nullptr && state->wasPressedThisFrame;
}

bool PlayerInputComponent::WasActionReleasedThisFrame(const char* const actionName) const noexcept {
    const InputActionRuntimeState* state = FindRuntimeState(actionName);
    return state != nullptr && state->wasReleasedThisFrame;
}

float PlayerInputComponent::GetActionAxis1D(const char* const actionName) const noexcept {
    const InputActionRuntimeState* state = FindRuntimeState(actionName);
    return state != nullptr ? state->axisValue : 0.0F;
}

}  // namespace Spark
