#include "spark/ecs/components/animation/AnimationLayerStackComponent.hpp"

#include "spark/animation/AnimLoopMode.hpp"
#include "spark/animation/Skeleton.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/GameObject.hpp"

namespace Spark {

void AnimationLayerStackComponent::PlayOverlay(const std::uint32_t clipIndex, const AnimLoopMode loopMode) noexcept {
    overlayClip = clipIndex;
    overlayLoop = loopMode;
    overlayTimeSeconds = 0.0F;
    overlayFinished = false;
    overlayActive = true;
}

void AnimationLayerStackComponent::ClearOverlay() noexcept {
    overlayActive = false;
    overlayFinished = false;
    overlayTimeSeconds = 0.0F;
}

void AnimationLayerStackComponent::ConfigureUpperBodyMaskFromSpine(
        const Skeleton& skeleton,
        const char* spineNameHint) noexcept {
    (void)jointMask.TryBuildUpperBodyFromSpineName(skeleton, spineNameHint);
}

void AnimationLayerStackComponent::AdvanceOverlayTime_(const float deltaSeconds, const Skeleton& skeleton) {
    if (!overlayActive || overlayFinished) {
        return;
    }
    if (overlayClip >= skeleton.GetClipCount()) {
        overlayFinished = true;
        return;
    }
    const float duration = skeleton.GetClipDuration(overlayClip);
    overlayTimeSeconds += deltaSeconds * overlaySpeed;
    if (overlayLoop == AnimLoopMode::Loop && duration > 1.0e-4F) {
        while (overlayTimeSeconds >= duration) {
            overlayTimeSeconds -= duration;
        }
        return;
    }
    if (duration <= 1.0e-4F || overlayTimeSeconds >= duration) {
        overlayTimeSeconds = (overlayLoop == AnimLoopMode::Hold && duration > 1.0e-4F) ? duration : overlayTimeSeconds;
        overlayFinished = true;
    }
}

void AnimationLayerStackComponent::OnUpdate(
        const FrameTiming& timing,
        GameObject& owner,
        IEngineContext& /*context*/) {
    const AnimatorComponent* animator = owner.GetComponent<AnimatorComponent>();
    if (animator == nullptr || !animator->GetSkeleton()) {
        return;
    }
    AdvanceOverlayTime_(timing.deltaTimeSeconds, *animator->GetSkeleton());
}

}  // namespace Spark
