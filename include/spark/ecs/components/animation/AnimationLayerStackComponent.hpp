#pragma once

#include "spark/animation/AnimLoopMode.hpp"
#include "spark/animation/AnimationJointMask.hpp"
#include "spark/ecs/GameComponent.hpp"

#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

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

    SPARK_SCRIPT_BIND(play_overlay)
    void PlayOverlay(std::uint32_t clipIndex, AnimLoopMode loopMode) noexcept;
    SPARK_SCRIPT_BIND(clear_overlay)
    void ClearOverlay() noexcept;

    SPARK_SCRIPT_BIND(is_overlay_active)
    [[nodiscard]] bool IsOverlayActive() const noexcept { return overlayActive; }
    SPARK_SCRIPT_BIND(is_overlay_finished)
    [[nodiscard]] bool IsOverlayFinished() const noexcept { return overlayActive && overlayFinished; }

    SPARK_SCRIPT_BIND(get_overlay_clip_index)
    [[nodiscard]] std::uint32_t GetOverlayClipIndex() const noexcept { return overlayClip; }
    SPARK_SCRIPT_BIND(get_overlay_time_seconds)
    [[nodiscard]] float GetOverlayTimeSeconds() const noexcept { return overlayTimeSeconds; }
    SPARK_SCRIPT_BIND(get_overlay_loop_mode)
    [[nodiscard]] AnimLoopMode GetOverlayLoopMode() const noexcept { return overlayLoop; }

    SPARK_SCRIPT_BIND(configure_upper_body_mask_from_spine)
    void ConfigureUpperBodyMaskFromSpine(const Skeleton& skeleton, const char* spineNameHint = "spine") noexcept;

    SPARK_SCRIPT_BIND(get_joint_mask)
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
