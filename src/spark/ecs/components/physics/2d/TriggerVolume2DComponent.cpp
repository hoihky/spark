#include "spark/ecs/components/physics/2d/TriggerVolume2DComponent.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/Signal.hpp"

namespace Spark {

namespace {

[[nodiscard]] bool ContainsId(const Array<std::uint64_t>& ids, const std::uint64_t id) noexcept {
    for (std::size_t i = 0; i < ids.GetSize(); ++i) {
        if (ids[i] == id) {
            return true;
        }
    }
    return false;
}

void EmitTriggerSignal(GameObject& owner, const SignalId id, GameObject& other) noexcept {
    SignalPayload payload{};
    payload.ptr = &other;
    payload.a = other.GetId();
    owner.EmitSignal(id, payload, nullptr);
}

}  // namespace

bool TriggerVolume2DComponent::IsOverlapping(const GameObject& other) const noexcept {
    return ContainsId(overlappingIds, other.GetId());
}

void TriggerVolume2DComponent::NotifyEnter(GameObject& other) {
    if (onEnter) {
        onEnter(other);
    }
    GameObject* owner = GetOwner();
    if (owner != nullptr) {
        EmitTriggerSignal(*owner, SignalId::Physics2DTriggerEnter, other);
    }
}

void TriggerVolume2DComponent::NotifyStay(GameObject& other) {
    if (onStay) {
        onStay(other);
    }
    GameObject* owner = GetOwner();
    if (owner != nullptr) {
        EmitTriggerSignal(*owner, SignalId::Physics2DTriggerStay, other);
    }
}

void TriggerVolume2DComponent::NotifyExit(GameObject& other) {
    if (onExit) {
        onExit(other);
    }
    GameObject* owner = GetOwner();
    if (owner != nullptr) {
        EmitTriggerSignal(*owner, SignalId::Physics2DTriggerExit, other);
    }
}

void TriggerVolume2DComponent::SetOverlappingIds(Array<std::uint64_t> ids) {
    overlappingIds = MoveTemp(ids);
}

}  // namespace Spark
