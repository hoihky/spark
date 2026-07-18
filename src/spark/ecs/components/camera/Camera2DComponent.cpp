#include "spark/ecs/components/camera/Camera2DComponent.hpp"

#include "spark/ecs/GameObject.hpp"

#include <cmath>

namespace Spark {

Camera2D Camera2DComponent::BuildCamera2D(const GameObject& owner) const noexcept {
    const Matrix4& world = owner.GetWorldMatrix();
    const Vector3 pos{world.m[12], world.m[13], world.m[14]};
    const float rx = world.m[0];
    const float ry = world.m[1];
    Camera2D cam{};
    cam.position = pos;
    cam.rotationRad = std::atan2(ry, rx);
    cam.halfExtentY = halfExtentY;
    cam.clipNearZ = clipNearZ;
    cam.clipFarZ = clipFarZ;
    return cam;
}

Matrix4 Camera2DComponent::ViewMatrix(const GameObject& owner) const noexcept {
    return BuildCamera2D(owner).ViewMatrix();
}

Matrix4 Camera2DComponent::ViewProjection(
        const GameObject& owner,
        const float framebufferWidth,
        const float framebufferHeight) const noexcept {
    return BuildCamera2D(owner).ViewProjection(framebufferWidth, framebufferHeight);
}

Vector3 Camera2DComponent::WorldPosition(const GameObject& owner) const noexcept {
    const Matrix4& world = owner.GetWorldMatrix();
    return {world.m[12], world.m[13], world.m[14]};
}

void Camera2DComponent::BillboardBasisWorld(
        const GameObject& owner,
        Vector3& outRight,
        Vector3& outUp) const noexcept {
    BuildCamera2D(owner).BillboardBasisWorld(outRight, outUp);
}

}  // namespace Spark
