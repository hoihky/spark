#pragma once

#include "spark/animation/AnimLoopMode.hpp"
#include "spark/animation/Skeleton.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/memory/SharedPtr.hpp"

#include <cstdint>

namespace Spark {

/**
 * Samples a shared Skeleton clip into joint palettes each frame (OnUpdate advances time).
 * Supports loop modes, clip crossfade, and clip lookup by name.
 * Sibling SkinnedMeshComponent provides geometry; Scene build reads palette via ComputeJointPalette.
 */
class AnimatorComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Animator;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override {
        return ComponentUpdatePriority::AnimatorPlayback;
    }

    AnimatorComponent(SharedPtr<Skeleton> inSkeleton, std::uint32_t clipIndex, float speed = 1.0F);

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    [[nodiscard]] const SharedPtr<Skeleton>& GetSkeleton() const noexcept { return skeleton; }
    [[nodiscard]] std::uint32_t GetClipIndex() const noexcept { return clipIndex; }
    [[nodiscard]] float GetTimeSeconds() const noexcept { return timeSeconds; }
    [[nodiscard]] float GetSpeed() const noexcept { return speed; }
    [[nodiscard]] AnimLoopMode GetLoopMode() const noexcept { return loopMode; }
    [[nodiscard]] bool IsClipFinished() const noexcept { return clipFinished; }
    [[nodiscard]] bool IsCrossfading() const noexcept { return crossfade.active; }

    [[nodiscard]] std::uint32_t GetClipCount() const noexcept;
    [[nodiscard]] const Utf8String& GetClipName(std::uint32_t clipIndex) const;
    [[nodiscard]] std::int32_t FindClipIndexByName(const char* name) const;

    void SetClipIndex(std::uint32_t c);
    void SetClipIndexWithCrossfade(std::uint32_t c, float crossfadeDurationSec);
    void SetSpeed(float s);
    void SetTimeSeconds(float t);
    void SetLoopMode(AnimLoopMode mode) noexcept;
    /** Resets playback to t=0 on the current clip (clears crossfade). */
    void RestartCurrentClip() noexcept;
    /** Swaps skeleton/clip (e.g. character model switch); resets playback state. */
    void RetargetSkeleton(SharedPtr<Skeleton> newSkeleton, std::uint32_t clipIndex, float newSpeed = 1.0F);

    /** Fills skin joint palette using loop mode, optional crossfade, and evaluated sample times. */
    void ComputeJointPalette(Matrix4* outPalette, std::uint32_t paletteMax) const;

private:
    void AdvancePrimaryTime_(float deltaSeconds);
    void AdvanceCrossfade_(float deltaSeconds);

    SharedPtr<Skeleton> skeleton;
    std::uint32_t clipIndex = 0;
    float timeSeconds = 0.0F;
    float speed = 1.0F;
    AnimLoopMode loopMode = AnimLoopMode::Loop;
    bool clipFinished = false;

    struct CrossfadeState {
        bool active = false;
        std::uint32_t fromClip = 0;
        float fromTime = 0.0F;
        float duration = 0.2F;
        float elapsed = 0.0F;
    };
    CrossfadeState crossfade{};
};

}  // namespace Spark
