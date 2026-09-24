#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

/** Maps a sprite animation marker name to a VFX asset played at the owner's world position. */
struct SpriteAnimationEventVfxBinding {
    Utf8String eventName;
    Utf8String vfxAssetKey;
};

/**
 * Listens for <c>SignalId::SpriteAnimationEvent</c> and queues matching VFX (Strategy per binding).
 * Pair with <c>SpriteAnimationEventReceiverComponent</c> on the same entity.
 */
class SpriteAnimationEventVfxComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::SpriteAnimationEventVfx;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] const Array<SpriteAnimationEventVfxBinding>& GetBindings() const noexcept { return bindings; }
    Array<SpriteAnimationEventVfxBinding>& GetBindings() noexcept { return bindings; }

    void ClearBindings() noexcept { bindings.Clear(); }
    void AddBinding(const char* eventName, const char* vfxAssetKey);

    void SetWorldOffset(const Vector3& offset) noexcept { worldOffset = offset; }
    [[nodiscard]] const Vector3& GetWorldOffset() const noexcept { return worldOffset; }

    void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override;

private:
    Array<SpriteAnimationEventVfxBinding> bindings{};
    Vector3 worldOffset{};
};

}  // namespace Spark
