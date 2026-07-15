#include "spark/scene/Camera2D.hpp"

#include "spark/math/Constants.hpp"

#include <cmath>

namespace Spark {

Matrix4 Camera2D::ViewMatrix() const noexcept {
    const Quaternion q = Quaternion::FromAxisAngle(Vector3::UnitZ, -rotationRad);
    const Matrix4 rot = Matrix4::Rotation(q);
    const Matrix4 trans = Matrix4::Translation({-position.x, -position.y, -position.z});
    return rot * trans;
}

Matrix4 Camera2D::ViewProjection(float framebufferWidth, float framebufferHeight) const noexcept {
    float h = framebufferHeight;
    if (h < Epsilon) {
        h = 1.0F;
    }
    float w = framebufferWidth;
    if (w < Epsilon) {
        w = h;
    }
    const float aspect = w / h;
    const float halfH = halfExtentY;
    const float halfW = halfH * aspect;
    const Matrix4 proj =
            Matrix4::OrthographicVulkan(-halfW, halfW, -halfH, halfH, clipNearZ, clipFarZ);
    return proj * ViewMatrix();
}

void Camera2D::BillboardBasisWorld(Vector3& outRight, Vector3& outUp) const noexcept {
    const float c = std::cos(rotationRad);
    const float s = std::sin(rotationRad);
    outRight = {c, s, 0.0F};
    outUp = {-s, c, 0.0F};
}

float RotationZRadFromQuaternion(const Quaternion& q) noexcept {
    const Quaternion n = q.Normalized();
    const float sinZ = 2.0F * (n.w * n.z + n.x * n.y);
    const float cosZ = 1.0F - 2.0F * (n.y * n.y + n.z * n.z);
    return -std::atan2(sinZ, cosZ);
}

Camera2D BuildCamera2D(
        const Vector3& worldPosition,
        const Quaternion& worldRotation,
        const float halfExtentYIn,
        const float clipNearZIn,
        const float clipFarZIn) noexcept {
    Camera2D cam{};
    cam.position = worldPosition;
    cam.rotationRad = RotationZRadFromQuaternion(worldRotation);
    cam.halfExtentY = halfExtentYIn;
    cam.clipNearZ = clipNearZIn;
    cam.clipFarZ = clipFarZIn;
    return cam;
}

}  // namespace Spark
