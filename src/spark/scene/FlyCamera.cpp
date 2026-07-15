#include "spark/scene/FlyCamera.hpp"

#include "spark/math/Constants.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

namespace Spark {

void FlyCamera::SnapLookAt(const Vector3& target) noexcept {
    Vector3 d = target - position;
    if (d.LengthSquared() < Epsilon) {
        return;
    }
    d = d.Normalized();
    pitch = std::asin(std::clamp(d.y, -1.0F, 1.0F));
    yaw = std::atan2(d.x, -d.z);
}

Vector3 FlyCamera::Forward() const noexcept {
    const float cy = std::cos(yaw);
    const float sy = std::sin(yaw);
    const float cp = std::cos(pitch);
    const float sp = std::sin(pitch);
    Vector3 f{sy * cp, sp, -cy * cp};
    return f.Normalized();
}

void FlyCamera::ProcessMovement(IInput& input, float deltaSeconds) noexcept {
    const Vector3 forward = Forward();
    Vector3 right = Vector3::Cross(forward, Vector3::UnitY);
    if (right.LengthSquared() < Epsilon) {
        right = Vector3::UnitX;
    } else {
        right = right.Normalized();
    }
    const float s = moveSpeed * deltaSeconds;
    if (input.IsKeyDown(GLFW_KEY_W)) {
        position += forward * s;
    }
    if (input.IsKeyDown(GLFW_KEY_S)) {
        position -= forward * s;
    }
    if (input.IsKeyDown(GLFW_KEY_A)) {
        position -= right * s;
    }
    if (input.IsKeyDown(GLFW_KEY_D)) {
        position += right * s;
    }
    if (input.IsKeyDown(GLFW_KEY_SPACE)) {
        position += Vector3::UnitY * s;
    }
    if (input.IsKeyDown(GLFW_KEY_LEFT_SHIFT)) {
        position -= Vector3::UnitY * s;
    }
}

Matrix4 FlyCamera::ViewMatrix() const noexcept {
    const Vector3 f = Forward();
    Vector3 worldUp = Vector3::UnitY;
    // When forward ≈ ±Y, Cross(f, UnitY) is tiny and LookAt becomes ill-conditioned; ground and
    // large planes can project incorrectly. Use a different up hint for steep pitch.
    const float align = std::fabs(Vector3::Dot(f, worldUp));
    if (align > 0.92F) {
        worldUp = Vector3::UnitZ;
    }
    const Vector3 target = position + f;
    return Matrix4::LookAt(position, target, worldUp);
}

}  // namespace Spark
