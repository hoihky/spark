#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/ecs/components/gameplay/GameStateComponent.hpp"

namespace Spark {

class GameObject;

/** Which world signal should advance game flow. */
enum class GameFlowTriggerSource : std::uint8_t {
    /** <c>SignalId::Physics2DTriggerEnter</c> on this entity's trigger volume. */
    TriggerEnter = 0,
    /** <c>SignalId::Died</c> on this entity (typically the player). */
    Died = 1,
    /** <c>SignalId::GameStateChanged</c> when another object reaches <c>watchState</c>. */
    OnGameState = 2,
};

/**
 * Bridges gameplay signals to a <c>GameStateComponent</c> (Mediator).
 * Attach to the game manager object or the entity that owns the state component.
 */
class GameFlowTriggerComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::GameFlowTrigger;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    void SetSource(GameFlowTriggerSource value) noexcept { source = value; }
    [[nodiscard]] GameFlowTriggerSource GetSource() const noexcept { return source; }

    void SetTargetState(const GameFlowState state) noexcept { targetState = state; }
    [[nodiscard]] GameFlowState GetTargetState() const noexcept { return targetState; }

    /** Optional filter for trigger-enter: only objects whose name contains this substring. */
    void SetInstigatorNameFilter(const char* filter) noexcept;
    [[nodiscard]] const char* GetInstigatorNameFilter() const noexcept { return instigatorNameFilter.CStr(); }

    void SetWatchState(const GameFlowState state) noexcept { watchState = state; }
    [[nodiscard]] GameFlowState GetWatchState() const noexcept { return watchState; }

    void SetStateOwner(GameObject* owner) noexcept { stateOwner = owner; }
    [[nodiscard]] GameObject* GetStateOwner() const noexcept { return stateOwner; }

    void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override;

private:
    [[nodiscard]] GameStateComponent* ResolveStateComponent(GameObject& owner) const noexcept;

    GameFlowTriggerSource source = GameFlowTriggerSource::TriggerEnter;
    GameFlowState targetState = GameFlowState::Victory;
    GameFlowState watchState = GameFlowState::Playing;
    Utf8String instigatorNameFilter{};
    GameObject* stateOwner = nullptr;
};

}  // namespace Spark
