#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/** Maps an animation marker name to a VFX asset played at the owner's world position. */
struct AnimationEventVfxBinding {
    Utf8String eventName;
    Utf8String vfxAssetKey;
};

/**
 * Listens for <c>SignalId::AnimationEvent</c> and queues <c>VfxPlayRequest</c> entries on matching bindings.
 * Pair with <c>AnimationEventReceiverComponent</c> on the same entity.
 */
class AnimationEventVfxComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::AnimationEventVfx;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] const Array<AnimationEventVfxBinding>& GetBindings() const noexcept { return bindings; }
    Array<AnimationEventVfxBinding>& GetBindings() noexcept { return bindings; }

    SPARK_SCRIPT_BIND(clear_bindings)
    void ClearBindings() noexcept { bindings.Clear(); }
    SPARK_SCRIPT_BIND(add_binding)
    void AddBinding(const char* eventName, const char* vfxAssetKey);

    SPARK_SCRIPT_BIND(set_world_offset)
    void SetWorldOffset(const Vector3& offset) noexcept { worldOffset = offset; }
    SPARK_SCRIPT_BIND(get_world_offset)
    [[nodiscard]] const Vector3& GetWorldOffset() const noexcept { return worldOffset; }

    void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override;

private:
    Array<AnimationEventVfxBinding> bindings{};
    Vector3 worldOffset{};
};

}  // namespace Spark
