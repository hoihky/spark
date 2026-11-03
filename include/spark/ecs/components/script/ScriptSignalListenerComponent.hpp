#pragma once

#include "spark/ecs/GameComponent.hpp"

namespace Spark {

class GameObject;

using ScriptSignalCallback = void (*)(void* userData, GameObject* owner, SignalId id, const SignalPayload& payload);

/**
 * Receives sibling signals on the same <c>GameObject</c> (standard <c>EmitSignal</c> dispatch).
 * Intended for script runtimes: register a C callback via interop.
 */
class ScriptSignalListenerComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::ScriptSignalListener;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    /** When <c>SignalId::None</c>, all signals are forwarded; otherwise only exact matches. */
    void SetListenSignal(const SignalId id) noexcept { listenSignal = id; }
    [[nodiscard]] SignalId GetListenSignal() const noexcept { return listenSignal; }

    void SetScriptCallback(const ScriptSignalCallback callback, void* userData) noexcept;

    void OnDetach(GameObject& owner) override;
    void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override;

private:
    SignalId listenSignal = SignalId::None;
    ScriptSignalCallback scriptCallback = nullptr;
    void* scriptUserData = nullptr;
};

}  // namespace Spark
