#include "spark/animation/ik/SkinnedIkPipeline.hpp"

#include "spark/animation/Skeleton.hpp"
#include "spark/ecs/components/animation/AimIkComponent.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/components/animation/FootIkComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"

#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] Vector3 HomogeneousMultiplyPoint(const Matrix4& matrix, const Vector3& point) noexcept {
    const Vector4 transformed = matrix * Vector4(point.x, point.y, point.z, 1.0F);
    const float w = (std::fabs(transformed.w) < 1.0e-8F) ? 1.0F : transformed.w;
    return {transformed.x / w, transformed.y / w, transformed.z / w};
}

}  // namespace

Vector3 SkinnedIkPipeline::ToWorldPosition(const Matrix4& ownerWorld, const Vector3& skeletonPosition) const noexcept {
    return HomogeneousMultiplyPoint(ownerWorld, skeletonPosition);
}

Vector3 SkinnedIkPipeline::ToSkeletonPosition(const Matrix4& ownerWorld, const Vector3& worldPosition) const {
    Matrix4 ownerInv{};
    if (!ownerWorld.TryInvert(ownerInv)) {
        return worldPosition;
    }
    return HomogeneousMultiplyPoint(ownerInv, worldPosition);
}

bool SkinnedIkPipeline::Apply(
        GameObject& owner,
        const Matrix4& ownerWorld,
        const AnimatorComponent& animator,
        Array<Transform>& pose) const {
    const SharedPtr<Skeleton>& skeleton = animator.GetSkeleton();
    if (!skeleton || pose.GetSize() < skeleton->GetJointCount()) {
        return false;
    }

    bool applied = false;
    if (const FootIkComponent* footIk = owner.GetComponent<FootIkComponent>()) {
        if (footIk->IsEnabled()) {
            ApplyFootIk(owner, ownerWorld, *skeleton, *footIk, pose);
            applied = true;
        }
    }
    if (const AimIkComponent* aimIk = owner.GetComponent<AimIkComponent>()) {
        if (aimIk->IsEnabled()) {
            ApplyAimIk(owner, ownerWorld, *skeleton, *aimIk, pose);
            applied = true;
        }
    }
    return applied;
}

void SkinnedIkPipeline::ApplyFootIk(
        GameObject& /*owner*/,
        const Matrix4& ownerWorld,
        const Skeleton& skeleton,
        const FootIkComponent& footIk,
        Array<Transform>& pose) const {
    if (groundProbe == nullptr || footIk.GetLimbs().IsEmpty()) {
        return;
    }

    for (std::size_t limbIndex = 0; limbIndex < footIk.GetLimbs().GetSize(); ++limbIndex) {
        const FootIkComponent::Limb& limb = footIk.GetLimbs()[limbIndex];
        if (limb.endJoint >= skeleton.GetJointCount()) {
            continue;
        }
        Matrix4 endWorld{};
        if (!skeleton.TryComputeJointWorldFromPose(pose, limb.endJoint, endWorld)) {
            continue;
        }
        const Vector3 footSkeleton = {endWorld.m[12], endWorld.m[13], endWorld.m[14]};
        const Vector3 footWorld = ToWorldPosition(ownerWorld, footSkeleton);
        const Vector3 poleHintSkeleton = footSkeleton + Vector3{limb.poleBiasX, 0.15F, -0.45F};

        IkGroundProbe::Hit groundHit{};
        const Vector3 rayOrigin = footWorld + Vector3{0.0F, footIk.GetRayOriginLift(), 0.0F};
        if (!groundProbe->ProbeDown(rayOrigin, footIk.GetRayMaxDistance() + footIk.GetRayOriginLift(), groundHit)
                || !groundHit.hasHit) {
            continue;
        }

        const Vector3 targetSkeleton = ToSkeletonPosition(ownerWorld, groundHit.point);
        footLimbSolver.Apply(skeleton, pose, limb, targetSkeleton, groundHit.normal, poleHintSkeleton, footIk.GetWeight());
    }
}

void SkinnedIkPipeline::ApplyAimIk(
        GameObject& /*owner*/,
        const Matrix4& ownerWorld,
        const Skeleton& skeleton,
        const AimIkComponent& aimIk,
        Array<Transform>& pose) const {
    if (aimIk.GetSpineJointIndices().IsEmpty()) {
        return;
    }

    Vector3 targetWorld = aimIk.GetWorldTarget();
    if (aimIk.UsesMainCamera()) {
        targetWorld = ToWorldPosition(ownerWorld, Vector3{0.0F, 1.35F, 4.0F});
    } else if (GameObject* targetObject = aimIk.GetTargetObject()) {
        if (targetObject->GetComponent<TransformComponent>() != nullptr) {
            targetWorld = targetObject->GetWorldMatrix().TranslationVector();
        }
    }

    const Vector3 targetSkeleton = ToSkeletonPosition(ownerWorld, targetWorld);
    spineAimSolver.ApplyChain(skeleton, pose, aimIk.GetSpineJointIndices(), targetSkeleton, aimIk.GetWeight());
}

}  // namespace Spark
