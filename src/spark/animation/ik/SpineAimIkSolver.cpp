#include "spark/animation/ik/SpineAimIkSolver.hpp"

#include "spark/animation/Skeleton.hpp"

#include <cmath>

namespace Spark {

Vector3 SpineAimIkSolver::ExtractBoneForward(const Matrix4& jointWorld) const noexcept {
    Vector3 forward{jointWorld.m[8], jointWorld.m[9], jointWorld.m[10]};
    const float lengthSquared = forward.LengthSquared();
    if (lengthSquared < 1.0e-8F) {
        return Vector3{0.0F, 0.0F, 1.0F};
    }
    return forward * (1.0F / std::sqrt(lengthSquared));
}

void SpineAimIkSolver::RotateJointToward(
        Transform& jointLocal,
        const Matrix4& parentWorld,
        const Vector3& jointWorldPosition,
        const Vector3& targetWorld,
        const float jointWeight) const {
    if (jointWeight <= 1.0e-4F) {
        return;
    }
    Matrix4 parentInv{};
    if (!parentWorld.TryInvert(parentInv)) {
        return;
    }

    Matrix4 jointWorld = parentWorld * jointLocal.ToMatrix4();
    const Vector3 currentForward = ExtractBoneForward(jointWorld);
    Vector3 desiredForward = targetWorld - jointWorldPosition;
    if (desiredForward.LengthSquared() < 1.0e-8F) {
        return;
    }
    desiredForward = desiredForward.Normalized();

    const Vector3 currentLocalForward = parentInv.TransformVector(currentForward).Normalized();
    const Vector3 desiredLocalForward = parentInv.TransformVector(desiredForward).Normalized();
    const Quaternion localDelta = Quaternion::FromShortestArc(currentLocalForward, desiredLocalForward);
    jointLocal.rotation =
            Quaternion::Slerp(jointLocal.rotation, localDelta * jointLocal.rotation, jointWeight).Normalized();
}

void SpineAimIkSolver::ApplyChain(
        const Skeleton& skeleton,
        Array<Transform>& pose,
        const Array<std::uint32_t>& jointIndices,
        const Vector3& targetWorld,
        const float weight) const {
    if (weight <= 1.0e-4F || jointIndices.IsEmpty() || pose.GetSize() < skeleton.GetJointCount()) {
        return;
    }

    const float perJointWeight = weight / static_cast<float>(jointIndices.GetSize());
    for (std::size_t index = 0; index < jointIndices.GetSize(); ++index) {
        const std::uint32_t jointIndex = jointIndices[index];
        if (jointIndex >= skeleton.GetJointCount()) {
            continue;
        }
        Matrix4 jointWorld{};
        if (!skeleton.TryComputeJointWorldFromPose(pose, jointIndex, jointWorld)) {
            continue;
        }
        const Vector3 jointPosition{jointWorld.m[12], jointWorld.m[13], jointWorld.m[14]};

        Matrix4 parentWorld = Matrix4::Identity;
        const std::int32_t parentIndex = skeleton.GetJointParent(jointIndex);
        if (parentIndex >= 0) {
            skeleton.TryComputeJointWorldFromPose(pose, static_cast<std::uint32_t>(parentIndex), parentWorld);
        }

        RotateJointToward(pose[jointIndex], parentWorld, jointPosition, targetWorld, perJointWeight);
    }
}

}  // namespace Spark
