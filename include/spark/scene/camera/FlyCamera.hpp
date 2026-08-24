#pragma once

#include "spark/engine/IInput.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

/**
 * FPS-style camera: WASD + mouse look, Space/Shift vertical. Expects PollEvents before BeginFrame each tick.
 */
struct FlyCamera {
    /** On +Z, farther back so the larger demo cube is not filling the frame. */
    Vector3 position{0.0F, 4.2F, 16.0F};
    float yaw = 0.0F;
    float pitch = 0.0F;
    float moveSpeed = 5.0F;
    float mouseSensitivity = 0.12F;

    /** Align yaw/pitch so Forward() matches (target - position). Call once after placing position. */
    void SnapLookAt(const Vector3& target) noexcept;

    void AddLook(float deltaX, float deltaY) noexcept {
        yaw += deltaX * mouseSensitivity * 0.01F;
        pitch -= deltaY * mouseSensitivity * 0.01F;
        constexpr float lim = 1.54F;
        if (pitch > lim) {
            pitch = lim;
        }
        if (pitch < -lim) {
            pitch = -lim;
        }
    }

    void ProcessMovement(IInput& input, float deltaSeconds) noexcept;

    [[nodiscard]] Vector3 Forward() const noexcept;
    [[nodiscard]] Matrix4 ViewMatrix() const noexcept;
};

}  // namespace Spark
