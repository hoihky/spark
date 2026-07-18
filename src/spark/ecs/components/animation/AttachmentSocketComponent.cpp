#include "spark/ecs/components/animation/AttachmentSocketComponent.hpp"

#include "spark/animation/Skeleton.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector4.hpp"

#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] Vector3 Hp3(const Vector4& p) noexcept {
    const float w = (std::fabs(p.w) < 1.0e-8F) ? 1.0F : p.w;
    return {p.x / w, p.y / w, p.z / w};
}

}  // namespace

void AttachmentSocketComponent::OnUpdate(
        const FrameTiming& /*timing*/,
        GameObject& owner,
        IEngineContext& /*context*/) {
    if (!enabled || attachedObject == nullptr) {
        return;
    }
    GameObject* source = sourceObject != nullptr ? sourceObject : owner.GetParent();
    if (source == nullptr) {
        source = &owner;
    }
    const AnimatorComponent* animator = source->GetComponent<AnimatorComponent>();
    TransformComponent* attachedTr = attachedObject->GetComponent<TransformComponent>();
    if (animator == nullptr || !animator->GetSkeleton() || attachedTr == nullptr) {
        return;
    }
    Matrix4 jointWorld{};
    if (!animator->GetSkeleton()->TryComputeJointWorldMatrix(
                animator->GetClipIndex(),
                animator->GetTimeSeconds(),
                jointIndex,
                jointWorld)) {
        return;
    }
    const Matrix4 socketWorld =
            source->GetWorldMatrix() * jointWorld * Matrix4::Translation(localOffset);
    const Vector3 worldPos = Hp3(socketWorld * Vector4(0.0F, 0.0F, 0.0F, 1.0F));

    Vector3 worldForward = {
            jointWorld.m[8],
            jointWorld.m[9],
            jointWorld.m[10]};
    const float fLen2 = worldForward.LengthSquared();
    if (fLen2 > 1.0e-8F) {
        worldForward = worldForward * (1.0F / std::sqrt(fLen2));
    } else {
        worldForward = Vector3{0.0F, 0.0F, 1.0F};
    }
    const Quaternion jointRot = Quaternion::FromShortestArc(Vector3{0.0F, 0.0F, 1.0F}, worldForward);
    const Quaternion worldRot = jointRot * localRotation;

    GameObject* parent = attachedObject->GetParent();
    if (parent == nullptr) {
        attachedTr->SetTranslation(worldPos);
        attachedTr->SetRotation(worldRot);
        return;
    }
    Matrix4 parentInv{};
    if (!parent->GetWorldMatrix().TryInvert(parentInv)) {
        return;
    }
    const Vector3 localPos = Hp3(parentInv * Vector4(worldPos.x, worldPos.y, worldPos.z, 1.0F));
    attachedTr->SetTranslation(localPos);
    attachedTr->SetRotation(worldRot);
}

}  // namespace Spark
