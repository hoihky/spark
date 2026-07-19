#include "spark/ecs/components/rendering/BillboardComponent.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/math/Constants.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/scene/Camera.hpp"
#include "spark/scene/GameWorld.hpp"

#include <cmath>

namespace Spark {

void BillboardComponent::OnUpdate(
        const FrameTiming& /*timing*/,
        GameObject& owner,
        IEngineContext& context) {
    if (!enabled) {
        return;
    }
    TransformComponent* tr = owner.GetComponent<TransformComponent>();
    if (tr == nullptr) {
        return;
    }
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    SceneCameraMatrices cam{};
    if (!TryBuildSceneCameraMatrices(owner.GetWorld(), static_cast<float>(fbW), static_cast<float>(fbH), cam)) {
        return;
    }
    const Vector3 camPos = cam.positionWorld;
    const Matrix4& worldM = owner.GetWorldMatrix();
    const Vector3 ownerPos{worldM.m[12], worldM.m[13], worldM.m[14]};
    Vector3 toCam = camPos - ownerPos;
    if (mode == BillboardMode::YAxisLocked) {
        toCam.y = 0.0F;
    }
    const float len2 = toCam.LengthSquared();
    if (len2 <= Epsilon) {
        return;
    }
    toCam = toCam * (1.0F / std::sqrt(len2));

    if (mode == BillboardMode::YAxisLocked) {
        const float yaw = std::atan2(toCam.x, toCam.z);
        tr->SetRotation(Quaternion::FromAxisAngle(Vector3::UnitY, yaw));
        return;
    }

    tr->SetRotation(Quaternion::FromShortestArc(Vector3{0.0F, 0.0F, 1.0F}, toCam));
}

}  // namespace Spark
