#pragma once

#include "spark/animation/IRootMotionApplicator.hpp"
#include "spark/animation/IRootMotionJointResolver.hpp"
#include "spark/animation/RootMotionSampler.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/UniquePtr.hpp"

#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class AnimatorComponent;
class GameObject;

/** Which axes of extracted root motion are applied to gameplay. */
enum class RootMotionTranslationMask : std::uint8_t {
    XZ = 0,
    Y = 1,
    XYZ = 2,
};

/**
 * Applies per-frame root-motion translation from a sibling/child <c>AnimatorComponent</c> to a
 * gameplay transform (or custom applicator). Runs after animator playback (priority 205).
 *
 * Typical setup: component on <c>characterRoot</c>, animator on skinned visual child, facing from root.
 */
class RootMotionComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::RootMotion;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override { return 205; }

    RootMotionComponent();

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    SPARK_SCRIPT_BIND(set_animator_object)
    void SetAnimatorObject(GameObject* object) noexcept { animatorObject = object; }
    SPARK_SCRIPT_BIND(get_animator_object)
    [[nodiscard]] GameObject* GetAnimatorObject() const noexcept { return animatorObject; }

    SPARK_SCRIPT_BIND(set_apply_target)
    void SetApplyTarget(GameObject* object) noexcept { applyTarget = object; }
    SPARK_SCRIPT_BIND(get_apply_target)
    [[nodiscard]] GameObject* GetApplyTarget() const noexcept { return applyTarget; }

    SPARK_SCRIPT_BIND(set_facing_object)
    void SetFacingObject(GameObject* object) noexcept { facingObject = object; }
    SPARK_SCRIPT_BIND(get_facing_object)
    [[nodiscard]] GameObject* GetFacingObject() const noexcept { return facingObject; }

    /** Multiplies skeleton-space deltas (e.g. skinned visual uniform scale). */
    SPARK_SCRIPT_BIND(set_skeleton_space_scale)
    void SetSkeletonSpaceScale(const float scale) noexcept { skeletonSpaceScale = scale > 0.0F ? scale : 1.0F; }
    SPARK_SCRIPT_BIND(get_skeleton_space_scale)
    [[nodiscard]] float GetSkeletonSpaceScale() const noexcept { return skeletonSpaceScale; }

    SPARK_SCRIPT_BIND(set_in_place)
    void SetInPlace(const bool enabled) noexcept { inPlace = enabled; }
    SPARK_SCRIPT_BIND(is_in_place)
    [[nodiscard]] bool IsInPlace() const noexcept { return inPlace; }

    SPARK_SCRIPT_BIND(set_enabled)
    void SetEnabled(const bool enabled) noexcept { active = enabled; }
    SPARK_SCRIPT_BIND(is_enabled)
    [[nodiscard]] bool IsEnabled() const noexcept { return active; }

    SPARK_SCRIPT_BIND(set_translation_mask)
    void SetTranslationMask(const RootMotionTranslationMask mask) noexcept { translationMask = mask; }
    SPARK_SCRIPT_BIND(get_translation_mask)
    [[nodiscard]] RootMotionTranslationMask GetTranslationMask() const noexcept { return translationMask; }

    SPARK_SCRIPT_BIND(set_motion_joint_index)
    void SetMotionJointIndex(const std::uint32_t jointIndex);
    SPARK_SCRIPT_BIND(use_pattern_motion_joint_resolver)
    void UsePatternMotionJointResolver();
    void SetCustomJointResolver(UniquePtr<IRootMotionJointResolver> resolver);
    void SetCustomApplicator(UniquePtr<IRootMotionApplicator> applicator);

    SPARK_SCRIPT_BIND(get_last_delta_skeleton_space)
    [[nodiscard]] const Vector3& GetLastDeltaSkeletonSpace() const noexcept { return lastDeltaSkeletonSpace; }
    SPARK_SCRIPT_BIND(get_last_delta_world_space)
    [[nodiscard]] const Vector3& GetLastDeltaWorldSpace() const noexcept { return lastDeltaWorldSpace; }

private:
    [[nodiscard]] std::uint32_t ResolveMotionJointIndex(const Skeleton& skeleton) const noexcept;
    [[nodiscard]] Vector3 ToWorldDelta(const Vector3& skeletonDelta, const GameObject& owner) const noexcept;
    [[nodiscard]] Vector3 ApplyTranslationMask(const Vector3& worldDelta) const noexcept;

    enum class JointResolverMode : std::uint8_t {
        Pattern = 0,
        Fixed = 1,
        Custom = 2,
    };

    GameObject* animatorObject = nullptr;
    GameObject* applyTarget = nullptr;
    GameObject* facingObject = nullptr;
    JointResolverMode jointResolverMode = JointResolverMode::Pattern;
    FixedRootMotionJointResolver fixedJointResolver;
    PatternRootMotionJointResolver patternJointResolver;
    UniquePtr<IRootMotionJointResolver> customJointResolver;
    UniquePtr<IRootMotionApplicator> customApplicator;
    TransformRootMotionApplicator defaultApplicator;
    RootMotionSampler sampler;
    Vector3 previousJointPosition{Vector3::Zero};
    Vector3 lastDeltaSkeletonSpace{Vector3::Zero};
    Vector3 lastDeltaWorldSpace{Vector3::Zero};
    float skeletonSpaceScale = 1.0F;
    bool hasPreviousSample = false;
    bool inPlace = false;
    bool active = true;
    RootMotionTranslationMask translationMask = RootMotionTranslationMask::XZ;
};

}  // namespace Spark
