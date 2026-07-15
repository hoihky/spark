#include "spark/scene/CharacterCameraRig.hpp"

#include "spark/engine/IInput.hpp"
#include "spark/math/Constants.hpp"
#include "spark/math/Quaternion.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

float WrapAnglePi(float a) noexcept {
    while (a > Pi) {
        a -= TwoPi;
    }
    while (a < -Pi) {
        a += TwoPi;
    }
    return a;
}

/**
 * Horizontal unit “out the front” for third-person camera / W,S. Same basis as demo root (qYaw * bind) ·
 * characterLocalForward, then negate in XZ so orbit/look match visible facing (glTF −Z is “front” but points
 * opposite the mesh nose after bind in this pipeline).
 */
Vector3 ThirdPersonFlatForward(const CharacterCameraRig& r) noexcept {
    const float yaw = WrapAnglePi(r.characterVisualYaw + r.characterFacingYawOffset);
    const Quaternion qYaw = Quaternion::FromAxisAngle(Vector3::UnitY, yaw);
    const Quaternion qTotal = (qYaw * r.characterRootBindOrientation).Normalized();
    Vector3 f = qTotal.RotateVector(r.characterLocalForward);
    const float hx = f.x;
    const float hz = f.z;
    const float hLen2 = hx * hx + hz * hz;
    if (hLen2 < 1.0e-12F) {
        const float sy = std::sin(yaw);
        const float cy = std::cos(yaw);
        return {-sy, 0.0F, cy};
    }
    const float inv = 1.0F / std::sqrt(hLen2);
    return {-hx * inv, 0.0F, -hz * inv};
}

}  // namespace

void CharacterCameraRig::ToggleCameraMode() noexcept {
    mode = (mode == CharacterCameraMode::FirstPerson) ? CharacterCameraMode::ThirdPerson
                                                      : CharacterCameraMode::FirstPerson;
    if (mode == CharacterCameraMode::ThirdPerson) {
        cameraPitch = std::clamp(cameraPitch, -0.48F, 0.52F);
        cameraYaw = characterVisualYaw;
    }
}

void CharacterCameraRig::AddLook(float deltaX, float deltaY) noexcept {
    const float dy = deltaX * mouseSensitivity * 0.01F;
    if (mode == CharacterCameraMode::ThirdPerson) {
        cameraYaw = WrapAnglePi(cameraYaw + dy);
        characterVisualYaw = WrapAnglePi(characterVisualYaw + dy);
    } else {
        cameraYaw = WrapAnglePi(cameraYaw + dy);
    }
    cameraPitch -= deltaY * mouseSensitivity * 0.01F;
    constexpr float limFp = 1.54F;
    constexpr float limTpMin = -0.48F;
    constexpr float limTpMax = 0.52F;
    if (mode == CharacterCameraMode::FirstPerson) {
        cameraPitch = std::clamp(cameraPitch, -limFp, limFp);
    } else {
        cameraPitch = std::clamp(cameraPitch, limTpMin, limTpMax);
    }
}

void CharacterCameraRig::ProcessWalk(IInput& input, float deltaSeconds) noexcept {
    if (mode == CharacterCameraMode::ThirdPerson) {
        const Vector3 flatF = ThirdPersonFlatForward(*this);

        Vector3 move{};
        if (input.IsKeyDown(GLFW_KEY_W)) {
            move += flatF;
        }
        if (input.IsKeyDown(GLFW_KEY_S)) {
            move -= flatF;
        }

        if (move.LengthSquared() > Epsilon) {
            const bool sprint = input.IsKeyDown(GLFW_KEY_LEFT_SHIFT) || input.IsKeyDown(GLFW_KEY_RIGHT_SHIFT);
            const float speed = (sprint ? runSpeed : moveSpeed) * deltaSeconds;
            move = move.Normalized() * speed;
            characterPosition += move;
        }
        characterPosition.y = groundY;
        return;
    }

    // First person: camera-relative WASD on XZ.
    const float sy = std::sin(cameraYaw);
    const float cy = std::cos(cameraYaw);
    Vector3 flatF{sy, 0.0F, -cy};
    Vector3 flatR = Vector3::Cross(flatF, Vector3::UnitY);
    if (flatR.LengthSquared() < Epsilon) {
        flatR = Vector3::UnitX;
    } else {
        flatR = flatR.Normalized();
    }

    Vector3 move{};
    if (input.IsKeyDown(GLFW_KEY_W)) {
        move += flatF;
    }
    if (input.IsKeyDown(GLFW_KEY_S)) {
        move -= flatF;
    }
    if (input.IsKeyDown(GLFW_KEY_D)) {
        move += flatR;
    }
    if (input.IsKeyDown(GLFW_KEY_A)) {
        move -= flatR;
    }

    if (move.LengthSquared() > Epsilon) {
        const bool sprint = input.IsKeyDown(GLFW_KEY_LEFT_SHIFT) || input.IsKeyDown(GLFW_KEY_RIGHT_SHIFT);
        const float speed = (sprint ? runSpeed : moveSpeed) * deltaSeconds;
        move = move.Normalized() * speed;
        characterPosition += move;
        characterVisualYaw = std::atan2(move.x, -move.z);
    }
    characterPosition.y = groundY;
}

Vector3 CharacterCameraRig::ForwardWorld() const noexcept {
    const float cyy = std::cos(cameraYaw);
    const float syy = std::sin(cameraYaw);
    const float cp = std::cos(cameraPitch);
    const float sp = std::sin(cameraPitch);
    Vector3 f{syy * cp, sp, -cyy * cp};
    return f.Normalized();
}

Vector3 CharacterCameraRig::FirstPersonEyeWorld() const noexcept {
    const float sy = std::sin(cameraYaw);
    const float cy = std::cos(cameraYaw);
    Vector3 flatFwd{sy, 0.0F, -cy};
    if (flatFwd.LengthSquared() < Epsilon) {
        flatFwd = Vector3{0.0F, 0.0F, -1.0F};
    } else {
        flatFwd = flatFwd.Normalized();
    }
    return characterPosition + Vector3::UnitY * firstPersonEyeHeight + flatFwd * firstPersonForwardNudge;
}

Vector3 CharacterCameraRig::CameraWorldPosition() const noexcept {
    if (mode == CharacterCameraMode::FirstPerson) {
        return FirstPersonEyeWorld();
    }
    const Vector3 pivot = characterPosition + Vector3::UnitY * thirdPersonPivotHeight;
    const Vector3 flatF = ThirdPersonFlatForward(*this);
    const float cp = std::cos(cameraPitch);
    const float sp = std::sin(cameraPitch);
    const float horiz = cp * thirdPersonDistance;
    const float vert = sp * thirdPersonDistance;
    return pivot - flatF * horiz + Vector3::UnitY * (vert + thirdPersonCameraLift);
}

Matrix4 CharacterCameraRig::ViewMatrix() const noexcept {
    if (mode == CharacterCameraMode::FirstPerson) {
        const Vector3 eye = FirstPersonEyeWorld();
        const Vector3 f = ForwardWorld();
        Vector3 worldUp = Vector3::UnitY;
        const float align = std::fabs(Vector3::Dot(f, worldUp));
        if (align > 0.92F) {
            worldUp = Vector3::UnitZ;
        }
        return Matrix4::LookAt(eye, eye + f, worldUp);
    }
    const Vector3 pivot = characterPosition + Vector3::UnitY * thirdPersonPivotHeight;
    const Vector3 flatF = ThirdPersonFlatForward(*this);
    const Vector3 focus = pivot + flatF * thirdPersonFocusAhead;
    const Vector3 cam = CameraWorldPosition();
    Vector3 worldUp = Vector3::UnitY;
    const Vector3 toFocus = (focus - cam).Normalized();
    const float align = std::fabs(Vector3::Dot(toFocus, worldUp));
    if (align > 0.96F) {
        worldUp = Vector3::UnitZ;
    }
    return Matrix4::LookAt(cam, focus, worldUp);
}

}  // namespace Spark
