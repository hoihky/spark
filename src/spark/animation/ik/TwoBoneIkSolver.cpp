#include "spark/animation/ik/TwoBoneIkSolver.hpp"

#include "spark/animation/Skeleton.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

Vector3 TwoBoneIkSolver::ExtractTranslation(const Matrix4& matrix) const noexcept {
    return {matrix.m[12], matrix.m[13], matrix.m[14]};
}

bool TwoBoneIkSolver::Apply(
        const Skeleton& skeleton,
        Array<Transform>& pose,
        const std::uint32_t rootJoint,
        const std::uint32_t midJoint,
        const std::uint32_t endJoint,
        const Vector3& targetWorld,
        const Vector3& poleHintWorld,
        const float weight) const {
    const std::uint32_t jointCount = skeleton.GetJointCount();
    if (weight <= 1.0e-4F || jointCount == 0 || pose.GetSize() < jointCount || rootJoint >= jointCount
            || midJoint >= jointCount || endJoint >= jointCount) {
        return false;
    }

    Matrix4 rootWorld{};
    Matrix4 midWorld{};
    Matrix4 endWorld{};
    if (!skeleton.TryComputeJointWorldFromPose(pose, rootJoint, rootWorld)
            || !skeleton.TryComputeJointWorldFromPose(pose, midJoint, midWorld)
            || !skeleton.TryComputeJointWorldFromPose(pose, endJoint, endWorld)) {
        return false;
    }

    const Vector3 rootPos = ExtractTranslation(rootWorld);
    const Vector3 midPos = ExtractTranslation(midWorld);
    const Vector3 endPos = ExtractTranslation(endWorld);
    const float upperLength = (midPos - rootPos).Length();
    const float lowerLength = (endPos - midPos).Length();
    if (upperLength < 1.0e-4F || lowerLength < 1.0e-4F) {
        return false;
    }

    Vector3 rootToTarget = targetWorld - rootPos;
    float targetDistance = rootToTarget.Length();
    const float maxReach = upperLength + lowerLength - 1.0e-3F;
    targetDistance = std::clamp(targetDistance, 1.0e-3F, maxReach);
    const Vector3 rootToTargetDir = rootToTarget * (1.0F / targetDistance);

    const float cosRoot =
            (upperLength * upperLength + targetDistance * targetDistance - lowerLength * lowerLength)
            / (2.0F * upperLength * targetDistance);
    const float clampedCos = std::clamp(cosRoot, -1.0F, 1.0F);
    const float sinRoot = std::sqrt(std::max(0.0F, 1.0F - clampedCos * clampedCos));

    Vector3 bendAxis = Vector3::Cross(rootToTargetDir, poleHintWorld - rootPos);
    if (bendAxis.LengthSquared() < 1.0e-6F) {
        bendAxis = Vector3::Cross(rootToTargetDir, Vector3::UnitX);
    }
    bendAxis = bendAxis.Normalized();
    const Vector3 bendDir = Vector3::Cross(bendAxis, rootToTargetDir).Normalized();

    const Vector3 solvedMid =
            rootPos + rootToTargetDir * (clampedCos * upperLength) + bendDir * (sinRoot * upperLength);
    const Vector3 solvedEnd = targetWorld;

    Matrix4 rootParentWorld = Matrix4::Identity;
    const std::int32_t rootParent = skeleton.GetJointParent(rootJoint);
    if (rootParent >= 0) {
        skeleton.TryComputeJointWorldFromPose(pose, static_cast<std::uint32_t>(rootParent), rootParentWorld);
    }

    Matrix4 parentInv{};
    if (rootParentWorld.TryInvert(parentInv)) {
        const Vector3 currentDir = (midPos - rootPos).Normalized();
        const Vector3 desiredDir = (solvedMid - rootPos).Normalized();
        const Vector3 currentLocalDir = parentInv.TransformVector(currentDir).Normalized();
        const Vector3 desiredLocalDir = parentInv.TransformVector(desiredDir).Normalized();
        const Quaternion localDelta = Quaternion::FromShortestArc(currentLocalDir, desiredLocalDir);
        Transform& rootLocal = pose[rootJoint];
        rootLocal.rotation =
                Quaternion::Slerp(rootLocal.rotation, localDelta * rootLocal.rotation, weight).Normalized();
    }

    skeleton.TryComputeJointWorldFromPose(pose, midJoint, midWorld);
    const Vector3 updatedMidPos = ExtractTranslation(midWorld);
    Matrix4 midParentWorld = Matrix4::Identity;
    const std::int32_t midParent = skeleton.GetJointParent(midJoint);
    if (midParent >= 0) {
        skeleton.TryComputeJointWorldFromPose(pose, static_cast<std::uint32_t>(midParent), midParentWorld);
    }
    if (midParentWorld.TryInvert(parentInv)) {
        const Vector3 currentDir = (endPos - updatedMidPos).Normalized();
        const Vector3 desiredDir = (solvedEnd - updatedMidPos).Normalized();
        const Vector3 currentLocalDir = parentInv.TransformVector(currentDir).Normalized();
        const Vector3 desiredLocalDir = parentInv.TransformVector(desiredDir).Normalized();
        const Quaternion localDelta = Quaternion::FromShortestArc(currentLocalDir, desiredLocalDir);
        Transform& midLocal = pose[midJoint];
        midLocal.rotation = Quaternion::Slerp(midLocal.rotation, localDelta * midLocal.rotation, weight).Normalized();
    }

    skeleton.TryComputeJointWorldFromPose(pose, endJoint, endWorld);
    const Vector3 updatedEndPos = ExtractTranslation(endWorld);
    Matrix4 endParentWorld = Matrix4::Identity;
    const std::int32_t endParent = skeleton.GetJointParent(endJoint);
    if (endParent >= 0) {
        skeleton.TryComputeJointWorldFromPose(pose, static_cast<std::uint32_t>(endParent), endParentWorld);
    }
    if (endParentWorld.TryInvert(parentInv)) {
        const Vector3 currentDir = (updatedEndPos - updatedMidPos).Normalized();
        const Vector3 desiredDir = (solvedEnd - updatedMidPos).Normalized();
        const Vector3 currentLocalDir = parentInv.TransformVector(currentDir).Normalized();
        const Vector3 desiredLocalDir = parentInv.TransformVector(desiredDir).Normalized();
        const Quaternion localDelta = Quaternion::FromShortestArc(currentLocalDir, desiredLocalDir);
        Transform& endLocal = pose[endJoint];
        endLocal.rotation = Quaternion::Slerp(endLocal.rotation, localDelta * endLocal.rotation, weight).Normalized();
    }

    return true;
}

}  // namespace Spark
