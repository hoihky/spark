#include "spark/animation/ik/FootIkLimbSolver.hpp"

#include "spark/animation/Skeleton.hpp"
#include "spark/animation/ik/TwoBoneIkSolver.hpp"

namespace Spark {

bool FootIkLimbSolver::Apply(
        const Skeleton& skeleton,
        Array<Transform>& pose,
        const FootIkComponent::Limb& limb,
        const Vector3& targetSkeleton,
        const Vector3& groundNormal,
        const Vector3& poleHintSkeleton,
        const float weight) const {
    if (weight <= 1.0e-4F || limb.endJoint >= skeleton.GetJointCount()) {
        return false;
    }
    if (limb.hasMidJoint && limb.midJoint < skeleton.GetJointCount() && limb.rootJoint < skeleton.GetJointCount()) {
        return twoBoneSolverRef.Apply(
                skeleton, pose, limb.rootJoint, limb.midJoint, limb.endJoint, targetSkeleton, poleHintSkeleton, weight);
    }

    Matrix4 endWorld{};
    if (!skeleton.TryComputeJointWorldFromPose(pose, limb.endJoint, endWorld)) {
        return false;
    }
    const Vector3 endPosition{endWorld.m[12], endWorld.m[13], endWorld.m[14]};
    Matrix4 parentWorld = Matrix4::Identity;
    const std::int32_t parentIndex = skeleton.GetJointParent(limb.endJoint);
    if (parentIndex >= 0) {
        skeleton.TryComputeJointWorldFromPose(pose, static_cast<std::uint32_t>(parentIndex), parentWorld);
    }

    Matrix4 parentInv{};
    if (!parentWorld.TryInvert(parentInv)) {
        return false;
    }

    Vector3 currentForward{endWorld.m[8], endWorld.m[9], endWorld.m[10]};
    if (currentForward.LengthSquared() < 1.0e-8F) {
        currentForward = Vector3{0.0F, -1.0F, 0.0F};
    } else {
        currentForward = currentForward.Normalized();
    }
    Vector3 desiredForward = groundNormal;
    if (desiredForward.LengthSquared() < 1.0e-8F) {
        desiredForward = Vector3::UnitY;
    } else {
        desiredForward = desiredForward.Normalized();
    }

    const Vector3 currentLocalForward = parentInv.TransformVector(currentForward).Normalized();
    const Vector3 desiredLocalForward = parentInv.TransformVector(desiredForward).Normalized();
    const Quaternion localDelta = Quaternion::FromShortestArc(currentLocalForward, desiredLocalForward);
    Transform& endLocal = pose[limb.endJoint];
    endLocal.rotation = Quaternion::Slerp(endLocal.rotation, localDelta * endLocal.rotation, weight).Normalized();

    const float verticalOffset = targetSkeleton.y - endPosition.y;
    endLocal.translation.y += verticalOffset * weight;
    return true;
}

}  // namespace Spark
