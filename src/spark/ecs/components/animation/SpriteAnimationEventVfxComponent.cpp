#include "spark/ecs/components/animation/SpriteAnimationEventVfxComponent.hpp"

#include "spark/core/Utility.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/Signal.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/vfx/VfxSubsystem.hpp"

#include <cstring>

namespace Spark {

void SpriteAnimationEventVfxComponent::AddBinding(const char* const eventName, const char* const vfxAssetKey) {
    SpriteAnimationEventVfxBinding binding{};
    binding.eventName = Utf8String(eventName != nullptr ? eventName : "");
    binding.vfxAssetKey = Utf8String(vfxAssetKey != nullptr ? vfxAssetKey : "");
    bindings.PushBack(MoveTemp(binding));
}

void SpriteAnimationEventVfxComponent::OnSignal(GameObject& owner, const SignalId id, const SignalPayload& payload) {
    if (id != SignalId::SpriteAnimationEvent || payload.ptr == nullptr || !owner.IsActiveInHierarchy()) {
        return;
    }
    const char* const eventName = static_cast<const char*>(payload.ptr);
    for (std::size_t i = 0; i < bindings.GetSize(); ++i) {
        const SpriteAnimationEventVfxBinding& binding = bindings[i];
        if (binding.eventName.IsEmpty() || binding.vfxAssetKey.IsEmpty()) {
            continue;
        }
        if (std::strcmp(binding.eventName.CStr(), eventName) != 0) {
            continue;
        }
        const Matrix4 wm = owner.GetWorldMatrix();
        const Vector3 position{
                wm.m[12] + worldOffset.x,
                wm.m[13] + worldOffset.y,
                wm.m[14] + worldOffset.z};
        owner.GetWorld().GetVfxSubsystem().Queue(binding.vfxAssetKey.CStr(), position);
    }
}

}  // namespace Spark
