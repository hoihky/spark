#include "spark/animation/locomotion/PlanarLocomotionKinematicsProvider.hpp"

#include "spark/animation/locomotion/LocomotionPlanarSample.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/physics/3d/CharacterController3DComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"

#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] float YawFromWorldMatrix(const Matrix4& world) noexcept {
    return std::atan2(world.m[8], world.m[10]);
}

}  // namespace

void PlanarLocomotionKinematicsProvider::FillSample(
        const GameObject& owner,
        LocomotionPlanarSample& outSample) const noexcept {
    const Matrix4 world = owner.GetWorldMatrix();
    outSample.SetBodyFacingYawRadians(YawFromWorldMatrix(world));

    float wishYaw = outSample.GetBodyFacingYawRadians();
    if (hasWishFacingOverride) {
        wishYaw = wishFacingYaw;
    }
    outSample.SetWishFacingYawRadians(wishYaw);

    Vector3 velocity{Vector3::Zero};
    for (const GameObject* node = &owner; node != nullptr; node = node->GetParent()) {
        if (const Rigidbody3DComponent* rb = node->GetComponent<Rigidbody3DComponent>()) {
            velocity = rb->GetVelocity();
            break;
        }
        if (const CharacterController3DComponent* controller =
                    node->GetComponent<CharacterController3DComponent>()) {
            velocity = controller->GetVelocity();
            break;
        }
    }
    outSample.SetVelocityWorldX(velocity.x);
    outSample.SetVelocityWorldZ(velocity.z);
}

}  // namespace Spark
