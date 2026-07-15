#pragma once

#include "spark/math/Matrix4.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

/**
 * Orthographic 2D camera in world XY (+Y up, +X right), compatible with SceneRenderParams::viewProjection.
 *
 * - Pan/orbit uses position (world point that stays at the screen center) and rotationRad around +Z.
 * - halfExtentY is half the visible world height; width follows framebuffer aspect (width/height).
 * - Depth: ortho maps view-space Z in [clipNearZ, clipFarZ] to [0,1] (same convention as Matrix4::OrthographicVulkan).
 *   Place 2D content at z=0 and use a clip range that contains 0 after ViewMatrix (e.g. clipNearZ=-500, clipFarZ=500
 *   with position.z=0).
 *
 * Screen vs world: framebuffer Y grows downward; after OrthographicVulkan, world +Y maps to increasing framebuffer Y.
 *
 * Sprite draw order for top-down scenes: use <c>SceneSpriteSortMode::SortOrderThenWorldY</c> on
 * <c>SubmitStandardLitSceneFromWorld</c> so characters with the same <c>sortOrder</c> occlude by world Y (+Y-up).
 */
struct Camera2D {
    Vector3 position{0.0F, 0.0F, 0.0F};
    /** Counter-clockwise rotation around +Z when looking down -Z (radians). */
    float rotationRad = 0.0F;
    /** Half of the visible height in world units (vertical span = 2 * halfExtentY). */
    float halfExtentY = 5.0F;
    float clipNearZ = -500.0F;
    float clipFarZ = 500.0F;

    [[nodiscard]] Matrix4 ViewMatrix() const noexcept;

    /**
     * Combined projection * view for the given framebuffer size in pixels (aspect = width/height).
     * Suitable for params.viewProjection and params.cameraPositionWorld = position.
     */
    [[nodiscard]] Matrix4 ViewProjection(float framebufferWidth, float framebufferHeight) const noexcept;

    /** World-space axes for billboards / particles when using this camera (normalized, right × up = +Z). */
    void BillboardBasisWorld(Vector3& outRight, Vector3& outUp) const noexcept;
};

/** Z-axis rotation (radians) encoded in a quaternion (matches <c>Camera2D::rotationRad</c> sign convention). */
[[nodiscard]] float RotationZRadFromQuaternion(const Quaternion& q) noexcept;

/** Builds a <c>Camera2D</c> from a world pose and projection fields. */
[[nodiscard]] Camera2D BuildCamera2D(
        const Vector3& worldPosition,
        const Quaternion& worldRotation,
        float halfExtentY,
        float clipNearZ = -500.0F,
        float clipFarZ = 500.0F) noexcept;

}  // namespace Spark
