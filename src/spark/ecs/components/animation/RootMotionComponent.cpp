#include "spark/ecs/components/animation/RootMotionComponent.hpp"

#include "spark/animation/AnimLoopMode.hpp"
#include "spark/core/Utility.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Quaternion.hpp"

namespace Spark {

RootMotionComponent::RootMotionComponent() = default;

void RootMotionComponent::SetMotionJointIndex(const std::uint32_t jointIndex) {
    fixedJointResolver.SetJointIndex(jointIndex);
    jointResolverMode = JointResolverMode::Fixed;
}

void RootMotionComponent::UsePatternMotionJointResolver() {
    jointResolverMode = JointResolverMode::Pattern;
}

void RootMotionComponent::SetCustomJointResolver(UniquePtr<IRootMotionJointResolver> resolver) {
    customJointResolver = MoveTemp(resolver);
    jointResolverMode = JointResolverMode::Custom;
}

void RootMotionComponent::SetCustomApplicator(UniquePtr<IRootMotionApplicator> applicator) {
    customApplicator = MoveTemp(applicator);
}

std::uint32_t RootMotionComponent::ResolveMotionJointIndex(const Skeleton& skeleton) const noexcept {
    switch (jointResolverMode) {
        case JointResolverMode::Fixed:
            return fixedJointResolver.ResolveJointIndex(skeleton);
        case JointResolverMode::Custom:
            if (customJointResolver != nullptr) {
                return customJointResolver->ResolveJointIndex(skeleton);
            }
            break;
        case JointResolverMode::Pattern:
        default:
            break;
    }
    return patternJointResolver.ResolveJointIndex(skeleton);
}

Vector3 RootMotionComponent::ToWorldDelta(const Vector3& skeletonDelta, const GameObject& owner) const noexcept {
    const Vector3 scaled{
            skeletonDelta.x * skeletonSpaceScale,
            skeletonDelta.y * skeletonSpaceScale,
            skeletonDelta.z * skeletonSpaceScale};

    const GameObject* basis = facingObject != nullptr ? facingObject : &owner;
    const Matrix4 world = basis->GetWorldMatrix();
    Quaternion facing = Quaternion::Identity;
    const Vector3 forward{-world.m[8], 0.0F, -world.m[10]};
    const float forwardLen2 = forward.LengthSquared();
    if (forwardLen2 > 1.0e-8F) {
        facing = Quaternion::FromShortestArc(Vector3{0.0F, 0.0F, -1.0F}, forward * (1.0F / std::sqrt(forwardLen2)));
    }
    return facing.RotateVector(scaled);
}

Vector3 RootMotionComponent::ApplyTranslationMask(const Vector3& worldDelta) const noexcept {
    switch (translationMask) {
        case RootMotionTranslationMask::Y:
            return {0.0F, worldDelta.y, 0.0F};
        case RootMotionTranslationMask::XYZ:
            return worldDelta;
        case RootMotionTranslationMask::XZ:
        default:
            return {worldDelta.x, 0.0F, worldDelta.z};
    }
}

void RootMotionComponent::OnUpdate(
        const FrameTiming& timing,
        GameObject& owner,
        IEngineContext& /*context*/) {
    lastDeltaSkeletonSpace = Vector3::Zero;
    lastDeltaWorldSpace = Vector3::Zero;

    if (!active) {
        hasPreviousSample = false;
        return;
    }

    GameObject* animatorOwner = animatorObject != nullptr ? animatorObject : &owner;
    const AnimatorComponent* animator = animatorOwner->GetComponent<AnimatorComponent>();
    if (animator == nullptr || !animator->GetSkeleton()) {
        hasPreviousSample = false;
        return;
    }

    if (animator->GetLoopMode() == AnimLoopMode::Once && animator->IsClipFinished()) {
        hasPreviousSample = false;
        return;
    }

    const std::uint32_t jointIndex = ResolveMotionJointIndex(*animator->GetSkeleton());
    RootMotionDelta delta{};
    if (!sampler.TryComputeDelta(
                *animator,
                jointIndex,
                timing.deltaTimeSeconds,
                previousJointPosition,
                hasPreviousSample,
                delta)) {
        return;
    }

    if (!delta.valid || inPlace) {
        return;
    }

    lastDeltaSkeletonSpace = delta.translation;
    const Vector3 worldDelta = ApplyTranslationMask(ToWorldDelta(delta.translation, owner));
    lastDeltaWorldSpace = worldDelta;

    GameObject* target = applyTarget != nullptr ? applyTarget : &owner;
    if (customApplicator != nullptr) {
        customApplicator->Apply(*target, worldDelta);
    } else {
        defaultApplicator.Apply(*target, worldDelta);
    }
}

}  // namespace Spark
