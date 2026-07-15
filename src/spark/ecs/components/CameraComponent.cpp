#include "spark/ecs/components/CameraComponent.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/math/Constants.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"

#include <cmath>

namespace Spark {

Matrix4 CameraComponent::ViewMatrix(const GameObject& owner) const noexcept {
    const Matrix4 world = owner.GetWorldMatrix();
    Matrix4 view{};
    if (!world.TryInvert(view)) {
        return Matrix4::Identity;
    }
    return view;
}

Matrix4 CameraComponent::ProjectionMatrix(const float aspect) const noexcept {
    const float safeAspect = (aspect > Epsilon) ? aspect : 1.0F;
    if (projectionMode == CameraProjectionMode::Orthographic) {
        const float halfH = orthoHalfHeight;
        const float halfW = halfH * safeAspect;
        return Matrix4::OrthographicVulkan(-halfW, halfW, -halfH, halfH, nearPlane, farPlane);
    }
    return Matrix4::PerspectiveVulkan(
            DegreesToRadians(fovYDegrees), safeAspect, nearPlane, farPlane);
}

Matrix4 CameraComponent::ViewProjection(const GameObject& owner, const float aspect) const noexcept {
    return ProjectionMatrix(aspect) * ViewMatrix(owner);
}

Vector3 CameraComponent::WorldPosition(const GameObject& owner) const noexcept {
    const Matrix4& world = owner.GetWorldMatrix();
    return {world.m[12], world.m[13], world.m[14]};
}

void CameraComponent::BillboardBasisWorld(
        const GameObject& owner,
        Vector3& outRight,
        Vector3& outUp) const noexcept {
    const Matrix4& world = owner.GetWorldMatrix();
    outRight = {world.m[0], world.m[1], world.m[2]};
    outUp = {world.m[4], world.m[5], world.m[6]};
    const float rLen2 = outRight.LengthSquared();
    if (rLen2 > Epsilon) {
        outRight = outRight * (1.0F / std::sqrt(rLen2));
    } else {
        outRight = Vector3::UnitX;
    }
    const float uLen2 = outUp.LengthSquared();
    if (uLen2 > Epsilon) {
        outUp = outUp * (1.0F / std::sqrt(uLen2));
    } else {
        outUp = Vector3::UnitY;
    }
}

}  // namespace Spark
