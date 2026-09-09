#include "spark/ecs/components/animation/SkeletonDebugDrawComponent.hpp"

#include "spark/animation/Skeleton.hpp"
#include "spark/core/Array.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector4.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] Vector3 HomogeneousPoint(const Matrix4& matrix, const Vector3& localPoint) noexcept {
    const Vector4 projected = matrix * Vector4(localPoint.x, localPoint.y, localPoint.z, 1.0F);
    const float w = (std::fabs(projected.w) < 1.0e-8F) ? 1.0F : projected.w;
    return {projected.x / w, projected.y / w, projected.z / w};
}

}  // namespace

void SkeletonDebugDrawComponent::AppendSceneDraws(Array<SceneDrawItem>& out) const {
    if (!enabled) {
        return;
    }

    GameObject* source = sourceObject;
    if (source == nullptr) {
        return;
    }
    const AnimatorComponent* animator = source->GetComponent<AnimatorComponent>();
    const SharedPtr<Skeleton>& skeleton = animator != nullptr ? animator->GetSkeleton() : SharedPtr<Skeleton>{};
    if (!animator || !skeleton || skeleton->GetJointCount() == 0) {
        return;
    }

    const Matrix4 sourceWorld = source->GetWorldMatrix();
    const std::uint32_t jointCount = skeleton->GetJointCount();
    Array<Vector3> jointWorldPositions;
    jointWorldPositions.Resize(jointCount);

    for (std::uint32_t jointIndex = 0; jointIndex < jointCount; ++jointIndex) {
        Matrix4 jointLocal{};
        if (!animator->TryComputeJointWorldMatrix(jointIndex, jointLocal)) {
            jointWorldPositions[jointIndex] = HomogeneousPoint(sourceWorld, Vector3::Zero);
            continue;
        }
        jointWorldPositions[jointIndex] = HomogeneousPoint(sourceWorld * jointLocal, Vector3::Zero);
    }

    for (std::uint32_t jointIndex = 0; jointIndex < jointCount; ++jointIndex) {
        AppendJointMarker(jointWorldPositions[jointIndex], out);
        const std::int32_t parentIndex = skeleton->GetJointParent(jointIndex);
        if (parentIndex < 0) {
            continue;
        }
        AppendBoneSegment(jointWorldPositions[static_cast<std::size_t>(parentIndex)], jointWorldPositions[jointIndex], out);
    }
}

void SkeletonDebugDrawComponent::AppendBoneSegment(
        const Vector3& fromWorld,
        const Vector3& toWorld,
        Array<SceneDrawItem>& out) const {
    const Vector3 delta{toWorld.x - fromWorld.x, toWorld.y - fromWorld.y, toWorld.z - fromWorld.z};
    const float length = delta.Length();
    if (length < 1.0e-5F) {
        return;
    }

    const Vector3 direction = delta * (1.0F / length);
    const float thickness = std::max(0.002F, boneThickness);
    const Quaternion rotation = Quaternion::FromShortestArc(Vector3::UnitY, direction);
    const Vector3 center{
            fromWorld.x + direction.x * length * 0.5F,
            fromWorld.y + direction.y * length * 0.5F,
            fromWorld.z + direction.z * length * 0.5F};

    SceneDrawItem segment{};
    segment.mesh = SceneMeshSlot::UnitCube;
    segment.model = Matrix4::Translation(center) * Matrix4::Rotation(rotation)
            * Matrix4::Scale({thickness, length, thickness});
    segment.albedo = lineColor;
    segment.metallic = 0.0F;
    segment.roughness = 0.35F;
    segment.emissiveColor = lineColor;
    segment.emissiveIntensity = 3.2F;
    out.PushBack(segment);
}

void SkeletonDebugDrawComponent::AppendJointMarker(const Vector3& positionWorld, Array<SceneDrawItem>& out) const {
    const float markerSize = std::max(0.004F, jointMarkerScale);
    SceneDrawItem marker{};
    marker.mesh = SceneMeshSlot::UnitCube;
    marker.model = Matrix4::Translation(positionWorld) * Matrix4::Scale(markerSize);
    marker.albedo = lineColor;
    marker.metallic = 0.0F;
    marker.roughness = 0.25F;
    marker.emissiveColor = {
            std::min(1.0F, lineColor.x + 0.15F),
            std::min(1.0F, lineColor.y + 0.15F),
            std::min(1.0F, lineColor.z + 0.15F)};
    marker.emissiveIntensity = 4.0F;
    out.PushBack(marker);
}

}  // namespace Spark
