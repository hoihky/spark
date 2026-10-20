#pragma once

#include "spark/animation/AnimLoopMode.hpp"
#include "spark/animation/AnimationJointMask.hpp"
#include "spark/ecs/GameComponent.hpp"

#include <cstdint>

namespace Spark {

class Skeleton;

/**
 * Upper-body (or masked) animation overlay while base <c>AnimatorComponent</c> keeps locomotion.
 * Evaluated inside <c>AnimatorComponent::TryComputeEvaluatedPose</c>.
 */
class AnimationLayerStackComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::AnimationLayerStack;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override { return 199; }

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    void PlayOverlay(std::uint32_t clipIndex, AnimLoopMode loopMode) noexcept;
    void ClearOverlay() noexcept;

    [[nodiscard]] bool IsOverlayActive() const noexcept { return overlayActive; }
    [[nodiscard]] bool IsOverlayFinished() const noexcept { return overlayActive && overlayFinished; }

    [[nodiscard]] std::uint32_t GetOverlayClipIndex() const noexcept { return overlayClip; }
    [[nodiscard]] float GetOverlayTimeSeconds() const noexcept { return overlayTimeSeconds; }
    [[nodiscard]] AnimLoopMode GetOverlayLoopMode() const noexcept { return overlayLoop; }

    void ConfigureUpperBodyMaskFromSpine(const Skeleton& skeleton, const char* spineNameHint = "spine") noexcept;

    [[nodiscard]] const AnimationJointMask& GetJointMask() const noexcept { return jointMask; }

private:
    void AdvanceOverlayTime_(float deltaSeconds, const Skeleton& skeleton);

    bool overlayActive = false;
    bool overlayFinished = false;
    std::uint32_t overlayClip = 0;
    float overlayTimeSeconds = 0.0F;
    float overlaySpeed = 1.0F;
    AnimLoopMode overlayLoop = AnimLoopMode::Once;
    AnimationJointMask jointMask{};
};

}  // namespace Spark
