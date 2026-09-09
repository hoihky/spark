#pragma once

#include "spark/animation/AnimLoopMode.hpp"
#include "spark/animation/Skeleton.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/memory/SharedPtr.hpp"

#include <cstdint>

namespace Spark {

class SkeletonPaletteCacheKey;

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
    [[nodiscard]] float GetCrossfadeBlend01() const noexcept {
        if (!crossfade.active || crossfade.duration <= 1.0e-4F) {
            return 1.0F;
        }
        const float blend = crossfade.elapsed / crossfade.duration;
        return (blend < 0.0F) ? 0.0F : (blend > 1.0F ? 1.0F : blend);
    }
    [[nodiscard]] std::uint32_t GetCrossfadeFromClip() const noexcept { return crossfade.fromClip; }
    [[nodiscard]] float GetCrossfadeFromTime() const noexcept { return crossfade.fromTime; }
    [[nodiscard]] bool IsLocomotionBlending() const noexcept { return locomotionBlend.active; }
    [[nodiscard]] std::uint32_t GetLocomotionBlendClipA() const noexcept { return locomotionBlend.clipA; }
    [[nodiscard]] std::uint32_t GetLocomotionBlendClipB() const noexcept { return locomotionBlend.clipB; }
    [[nodiscard]] float GetLocomotionBlend01() const noexcept { return locomotionBlend.blend01; }

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

    /**
     * Dual-clip locomotion blend (walk↔run). Takes precedence over single-clip playback when active.
     * Both clips advance with the same playback time. Clears crossfade.
     */
    void SetLocomotionBlend(std::uint32_t clipA, std::uint32_t clipB, float blend01);
    void ClearLocomotionBlend() noexcept;

    /** Fills skin joint palette using loop mode, optional crossfade/blend, and evaluated sample times. */
    void ComputeJointPalette(Matrix4* outPalette, std::uint32_t paletteMax) const;

    /** Samples the current animated pose (crossfade / locomotion blend aware). */
    [[nodiscard]] bool TrySampleEvaluatedPose(Array<Transform>& outPose) const;

    /**
     * Joint world matrix in skeleton space using the same crossfade / locomotion-blend rules as
     * <c>ComputeJointPalette</c>.
     */
    [[nodiscard]] bool TryComputeJointWorldMatrix(std::uint32_t jointIndex, Matrix4& outJointWorld) const;

    friend class SkeletonPaletteCacheKey;

private:
    void AdvancePrimaryTime_(float deltaSeconds);
    void AdvanceCrossfade_(float deltaSeconds);
    [[nodiscard]] bool TryComputeEvaluatedPose(Array<Transform>& outPose) const;

    SharedPtr<Skeleton> skeleton;
    std::uint32_t clipIndex = 0;
    float timeSeconds = 0.0F;
    float speed = 1.0F;
    AnimLoopMode loopMode = AnimLoopMode::Loop;
    bool clipFinished = false;

    struct CrossfadeState {
        bool active = false;
        bool fromLocomotionBlend = false;
        std::uint32_t fromClip = 0;
        float fromTime = 0.0F;
        std::uint32_t fromBlendClipA = 0;
        std::uint32_t fromBlendClipB = 0;
        float fromBlend01 = 0.0F;
        float duration = 0.2F;
        float elapsed = 0.0F;
    };
    CrossfadeState crossfade{};

    struct LocomotionBlendState {
        bool active = false;
        std::uint32_t clipA = 0;
        std::uint32_t clipB = 0;
        float blend01 = 0.0F;
    };
    LocomotionBlendState locomotionBlend{};
};

}  // namespace Spark
