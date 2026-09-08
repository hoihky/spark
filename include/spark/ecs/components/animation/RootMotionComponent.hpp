#pragma once

#include "spark/animation/IRootMotionApplicator.hpp"
#include "spark/animation/IRootMotionJointResolver.hpp"
#include "spark/animation/RootMotionSampler.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/UniquePtr.hpp"

#include <cstdint>

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

    void SetAnimatorObject(GameObject* object) noexcept { animatorObject = object; }
    [[nodiscard]] GameObject* GetAnimatorObject() const noexcept { return animatorObject; }

    void SetApplyTarget(GameObject* object) noexcept { applyTarget = object; }
    [[nodiscard]] GameObject* GetApplyTarget() const noexcept { return applyTarget; }

    void SetFacingObject(GameObject* object) noexcept { facingObject = object; }
    [[nodiscard]] GameObject* GetFacingObject() const noexcept { return facingObject; }

    /** Multiplies skeleton-space deltas (e.g. skinned visual uniform scale). */
    void SetSkeletonSpaceScale(const float scale) noexcept { skeletonSpaceScale = scale > 0.0F ? scale : 1.0F; }
    [[nodiscard]] float GetSkeletonSpaceScale() const noexcept { return skeletonSpaceScale; }

    void SetInPlace(const bool enabled) noexcept { inPlace = enabled; }
    [[nodiscard]] bool IsInPlace() const noexcept { return inPlace; }

    void SetEnabled(const bool enabled) noexcept { active = enabled; }
    [[nodiscard]] bool IsEnabled() const noexcept { return active; }

    void SetTranslationMask(const RootMotionTranslationMask mask) noexcept { translationMask = mask; }
    [[nodiscard]] RootMotionTranslationMask GetTranslationMask() const noexcept { return translationMask; }

    void SetMotionJointIndex(const std::uint32_t jointIndex);
    void UsePatternMotionJointResolver();
    void SetCustomJointResolver(UniquePtr<IRootMotionJointResolver> resolver);
    void SetCustomApplicator(UniquePtr<IRootMotionApplicator> applicator);

    [[nodiscard]] const Vector3& GetLastDeltaSkeletonSpace() const noexcept { return lastDeltaSkeletonSpace; }
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
