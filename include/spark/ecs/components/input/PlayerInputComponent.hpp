#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/input/InputActionTypes.hpp"

#include <cstdint>

namespace Spark {

class GameObject;
class IInput;
class InputActionMapComponent;

/** Per-action runtime state updated each frame by <c>PlayerInputComponent</c>. */
struct InputActionRuntimeState {
    Utf8String name;
    bool isPressed = false;
    bool wasPressedThisFrame = false;
    bool wasReleasedThisFrame = false;
    float axisValue = 0.0F;
};

/**
 * Polls <c>IInput</c> through a sibling <c>InputActionMapComponent</c> and exposes semantic queries.
 * Emits <c>SignalId::InputActionTriggered</c> on action edges (Command / Observer).
 */
class PlayerInputComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::PlayerInput;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override { return 50; }

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    /** Polls hardware input immediately (call before gameplay when update order is custom). */
    void Refresh(GameObject& owner, IEngineContext& context);

    void SetActionMap(InputActionMapComponent* mapIn) noexcept { actionMap = mapIn; }
    [[nodiscard]] InputActionMapComponent* GetActionMap() const noexcept { return actionMap; }

    [[nodiscard]] bool IsActionPressed(const char* actionName) const noexcept;
    [[nodiscard]] bool WasActionPressedThisFrame(const char* actionName) const noexcept;
    [[nodiscard]] bool WasActionReleasedThisFrame(const char* actionName) const noexcept;
    [[nodiscard]] float GetActionAxis1D(const char* actionName) const noexcept;

private:
    InputActionMapComponent* actionMap = nullptr;
    Array<InputActionRuntimeState> runtimeStates{};

    [[nodiscard]] InputActionRuntimeState* FindRuntimeState(const char* actionName) noexcept;
    [[nodiscard]] const InputActionRuntimeState* FindRuntimeState(const char* actionName) const noexcept;
    void SyncRuntimeStates(const InputActionMapComponent& map);
    void PollAction(
            GameObject& owner,
            const InputActionDefinition& definition,
            InputActionRuntimeState& state,
            IInput& input) noexcept;
};

}  // namespace Spark
