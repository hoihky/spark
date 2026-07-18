#include "spark/ai/PerceptionSubsystem.hpp"

#include "spark/ecs/components/ai/PerceptionSensorComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/GameWorld.hpp"

#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] bool HasMatchingCategory(const GameObject& /*target*/, const std::uint16_t mask) noexcept {
    if (mask == 0xFFFFu) {
        return true;
    }
    return true;
}

[[nodiscard]] bool InSightCone(
        const Vector3& sensorPos,
        const Vector3& sensorForward,
        const Vector3& targetPos,
        const float radius,
        const float halfFovRad) noexcept {
    const Vector3 delta = targetPos - sensorPos;
    const float dist2 = delta.LengthSquared();
    if (dist2 > radius * radius) {
        return false;
    }
    if (dist2 <= 1.0e-8F) {
        return true;
    }
    const Vector3 dir = delta * (1.0F / std::sqrt(dist2));
    const float dot = Vector3::Dot(sensorForward, dir);
    return dot >= std::cos(halfFovRad);
}

}  // namespace

void ProcessPerceptionSensors(GameWorld& world) noexcept {
    world.ForEachActiveGameObject([&](GameObject* sensorObj) {
        if (sensorObj == nullptr) {
            return;
        }
        PerceptionSensorComponent* sensor = sensorObj->GetComponent<PerceptionSensorComponent>();
        if (sensor == nullptr || !sensor->IsEnabled()) {
            return;
        }
        const TransformComponent* sensorTr = sensorObj->GetComponent<TransformComponent>();
        if (sensorTr == nullptr) {
            return;
        }
        sensor->ClearDetected();
        const Vector3 sensorPos = sensorObj->GetWorldMatrix().TranslationVector();
        Vector3 sensorForward = sensorObj->GetWorldMatrix().TransformVector(Vector3{0.0F, 0.0F, -1.0F});
        if (sensorForward.LengthSquared() > 1.0e-8F) {
            sensorForward = sensorForward.Normalized();
        } else {
            sensorForward = Vector3{0.0F, 0.0F, -1.0F};
        }
        const float halfFov = sensor->GetSightFovDegrees() * (3.14159265F / 360.0F);
        const float hearR = sensor->GetHearingRadius();
        const float hearR2 = hearR * hearR;

        world.ForEachActiveGameObject([&](GameObject* other) {
            if (other == nullptr || other == sensorObj) {
                return;
            }
            if (!HasMatchingCategory(*other, sensor->GetTargetCategoryMask())) {
                return;
            }
            const TransformComponent* otherTr = other->GetComponent<TransformComponent>();
            if (otherTr == nullptr) {
                return;
            }
            const Vector3 otherPos = other->GetWorldMatrix().TranslationVector();
            const Vector3 delta = otherPos - sensorPos;
            const float dist2 = delta.LengthSquared();
            if (InSightCone(sensorPos, sensorForward, otherPos, sensor->GetSightRadius(), halfFov)) {
                sensor->AddDetected(other);
                return;
            }
            if (dist2 <= hearR2) {
                sensor->AddDetected(other);
            }
        });
    });
}

}  // namespace Spark
