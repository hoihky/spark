#include "spark/ecs/components/physics/3d/TriggerVolume3DComponent.hpp"

#include "spark/core/Utility.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/Signal.hpp"

#include <cstring>

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

bool TriggerVolume3DComponent::IsOverlapping(const GameObject& other) const noexcept {
    return ContainsId(overlappingIds, other.GetId());
}

void TriggerVolume3DComponent::NotifyEnter(GameObject& other) {
    if (onEnter) {
        onEnter(other);
    }
    GameObject* owner = GetOwner();
    if (owner != nullptr) {
        EmitTriggerSignal(*owner, SignalId::Physics3DTriggerEnter, other);
    }
}

void TriggerVolume3DComponent::NotifyExit(GameObject& other) {
    if (onExit) {
        onExit(other);
    }
    GameObject* owner = GetOwner();
    if (owner != nullptr) {
        EmitTriggerSignal(*owner, SignalId::Physics3DTriggerExit, other);
    }
}

void TriggerVolume3DComponent::SetOverlappingIds(Array<std::uint64_t> ids) {
    overlappingIds = MoveTemp(ids);
}

}  // namespace Spark
