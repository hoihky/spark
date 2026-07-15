#pragma once

#include <cstdint>

#include "spark/math/Matrix4.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

class IInput;

/** First-person: camera-relative WASD. Third-person: follow behind character; A/D unused. */
enum class CharacterCameraMode : std::uint8_t {
    FirstPerson,
    ThirdPerson,
};

/**
 * First-person: camera at eye; WASD strafes relative to camera yaw; mouse looks.
 * Third-person: camera behind character, slightly above head; mouse yaw rotates character and camera together;
 * W/S move along character facing; A/D have no effect (use mouse to turn).
 */
struct CharacterCameraRig {
    CharacterCameraMode mode = CharacterCameraMode::ThirdPerson;

    /** Feet position; Y is locked to @ref groundY each tick. */
    Vector3 characterPosition{0.0F, 0.02F, 2.5F};
    /** Y-axis rotation from input (radians). Third-person camera orbit uses this plus @ref characterFacingYawOffset. */
    float characterVisualYaw = 0.0F;
    /**
     * Extra yaw added for world heading (third-person orbit, W/S, look target), e.g. glTF bindFacingYawOffset on
     * the mesh. Mouse adjusts @ref characterVisualYaw only; effective heading = sum (wrapped).
     */
    float characterFacingYawOffset = 0.0F;
    /**
     * Same quaternion multiplied after yaw on the character root (e.g. skinned glTF bind-up fix). Third-person
     * forward is derived from (yaw quat * this) · characterLocalForward so camera matches mesh facing.
     */
    Quaternion characterRootBindOrientation{Quaternion::Identity};
    /** Root-local axis fed into third-person forward (with pipeline negation); default glTF-style −Z. */
    Vector3 characterLocalForward{0.0F, 0.0F, -1.0F};

    float cameraYaw = 0.0F;
    float cameraPitch = 0.0F;

    float thirdPersonDistance = 4.25F;
    /** Look target height above feet (head / upper chest); third-person camera looks near here + @ref thirdPersonFocusAhead. */
    float thirdPersonPivotHeight = 1.55F;
    /** World-space offset along character forward for the look target (eyes “line of sight” point). */
    float thirdPersonFocusAhead = 0.35F;
    /** Extra world +Y on the camera so it sits a bit above the head while staying behind. */
    float thirdPersonCameraLift = 0.5F;
    /** Eye height above logical feet (world Y); tuned per character scale in demos. */
    float firstPersonEyeHeight = 1.62F;
    /** Pushes first-person eye along horizontal camera yaw so the view clears the torso/head mesh. */
    float firstPersonForwardNudge = 0.22F;

    float mouseSensitivity = 0.12F;
    float moveSpeed = 4.5F;
    float runSpeed = 8.5F;
    float groundY = 0.02F;

    void ToggleCameraMode() noexcept;

    void AddLook(float deltaX, float deltaY) noexcept;

    /** First person: WASD vs camera yaw. Third person: W/S along character yaw; A/D unused. */
    void ProcessWalk(IInput& input, float deltaSeconds) noexcept;

    [[nodiscard]] Vector3 ForwardWorld() const noexcept;
    /** First-person eye in world space (feet + eye height + forward nudge along yaw). */
    [[nodiscard]] Vector3 FirstPersonEyeWorld() const noexcept;
    [[nodiscard]] Vector3 CameraWorldPosition() const noexcept;
    [[nodiscard]] Matrix4 ViewMatrix() const noexcept;
};

}  // namespace Spark
